/**
 * flash.c — DSR-1 Settings Save / Restore
 *
 * WM-D6C Servo Replacement Module (DSR-1)
 * Hardware: STM32G0C1KCU6, bare-metal, no HAL, no RTOS
 *
 * Stores PI gain constants (Kp, Ki) and the master target period in the
 * last 2 KB flash page (0x0803F800–0x0803FFFF). This page lies outside the
 * linker's code region and is preserved across DFU updates.
 *
 * Flash organisation (STM32G0C1xC, 256 KB, RM0444 §3.3.1):
 *   The 256 KB device is dual-bank when the DUAL_BANK option bit is set
 *   (factory default): bank 1 = 0x08000000–0x0801FFFF, bank 2 =
 *   0x08020000–0x0803FFFF, 64 × 2 KB pages each. Bank 2 pages are selected
 *   with FLASH_CR.BKER = 1 and numbered from 256 in FLASH_CR.PNB, so the
 *   settings page is PNB = 256 + 63 = 319. flash_save() refuses to erase if
 *   DUAL_BANK has been cleared rather than guess the single-bank geometry.
 *   VERIFY on first bench save: confirm PNB/BKER encoding against RM0444.
 *
 * Layout of the settings block (32 bytes, aligned to 8 bytes for double-word
 * flash writes on STM32G0):
 *
 *   Offset  Size  Field
 *   0x00    4     magic       — SETTINGS_MAGIC sentinel ("DSR1")
 *   0x04    4     kp_q16      — Proportional gain in Q16 format
 *   0x08    4     ki_q16      — Integral gain in Q16 format
 *   0x0C    4     target_period — FG target period in 64 MHz ticks
 *   0x10    4     crc32       — CRC-32 of bytes 0x00–0x0F
 *   0x14    12    (reserved, written as 0xFFFFFFFF)
 *
 * On power-on, flash_load() checks magic and CRC. If both pass, saved
 * constants are loaded into g_kp_q16, g_ki_q16, and g_target_period.
 * If either fails, compiled-in defaults from config.h are used instead.
 *
 * Flash write procedure (STM32G0C1 RM0444 §3.3):
 *   1. Unlock flash (FLASH->KEYR)
 *   2. Erase the page (FLASH->CR PER + BKER + PNB + STRT, wait for BSY1/BSY2)
 *   3. Write data as 64-bit double-words (FLASH->CR PG, write 2×32-bit)
 *   4. Lock flash (FLASH->CR LOCK)
 *
 * The STM32G0C1 requires double-word (64-bit) writes — two consecutive
 * 32-bit words must be written before the internal programming latch fires.
 * Writing a single 32-bit word and not following with a second leaves the
 * latch in a partial state and corrupts the flash cell.
 *
 * CRC-32:
 *   Software CRC-32 (polynomial 0xEDB88320, ISO 3309 / Ethernet).
 *   The STM32G0C1 has a hardware CRC unit but it uses the MPEG-2 polynomial
 *   by default. Using software CRC here avoids claiming the CRC peripheral
 *   and keeps flash.c self-contained.
 */

#include "stm32g0xx.h"
#include <stdint.h>
#include <stddef.h>
#include "config.h"
#include "servo.h"
#include "flash.h"

/* -------------------------------------------------------------------------
 * Flash unlock keys (RM0444 §3.4.2)
 * ------------------------------------------------------------------------- */
#define FLASH_KEY1      0x45670123UL
#define FLASH_KEY2      0xCDEF89ABUL

/* Flash geometry on STM32G0C1xC: 256 KB, 2 KB pages, two 128 KB banks */
#define FLASH_PAGE_SIZE         2048U
#define FLASH_BASE_ADDR         0x08000000UL
#define FLASH_BANK_SIZE         (128UL * 1024UL)
#define FLASH_BANK2_PNB_BASE    256U    /* Bank 2 page n is PNB = 256 + n */

/* The settings block fits in one page; we erase and rewrite the whole page.
 * FLASH_SETTINGS_ADDR = 0x0803F800 → bank 2, page 63 → PNB 319, BKER = 1. */
_Static_assert((FLASH_SETTINGS_ADDR - FLASH_BASE_ADDR) >= FLASH_BANK_SIZE,
               "Settings page is expected to live in flash bank 2");
_Static_assert((FLASH_SETTINGS_ADDR % FLASH_PAGE_SIZE) == 0,
               "Settings address must be page-aligned");
#define SETTINGS_BANK2_PAGE  ((FLASH_SETTINGS_ADDR - FLASH_BASE_ADDR - FLASH_BANK_SIZE) / FLASH_PAGE_SIZE)

/* -------------------------------------------------------------------------
 * Settings block layout — must match the description in the file header.
 * __attribute__((packed)) ensures no padding inserted by the compiler.
 * ------------------------------------------------------------------------- */
typedef struct __attribute__((packed)) {
    uint32_t magic;
    int32_t  kp_q16;
    int32_t  ki_q16;
    uint32_t target_period;
    uint32_t crc32;
    uint32_t reserved[3];   /* Pad to 32 bytes, written as 0xFFFFFFFF */
} SettingsBlock;

_Static_assert(sizeof(SettingsBlock) == 32, "SettingsBlock must be exactly 32 bytes");
_Static_assert(sizeof(SettingsBlock) % 8 == 0, "SettingsBlock must be a multiple of 8 bytes for double-word flash writes");

/* -------------------------------------------------------------------------
 * crc32_compute()
 *
 * Software CRC-32 (ISO 3309 polynomial, same as Ethernet / zlib).
 * Operates on byte array of given length.
 * Returns 32-bit CRC.
 * ------------------------------------------------------------------------- */
static uint32_t crc32_compute(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 1U) {
                crc = (crc >> 1) ^ 0xEDB88320UL;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFFUL;
}

/* -------------------------------------------------------------------------
 * flash_unlock() / flash_lock()
 *
 * Unlock sequence writes the two key values in order to FLASH->KEYR.
 * Any incorrect value or out-of-order write triggers a hardware lock.
 * Lock is set by writing LOCK bit to FLASH->CR.
 * ------------------------------------------------------------------------- */
static void flash_unlock(void)
{
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
}

static void flash_lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

/* -------------------------------------------------------------------------
 * flash_wait_busy()
 *
 * Spins until the flash controller is idle (BSY = 0).
 * Must be called after any erase or program operation before proceeding.
 * ------------------------------------------------------------------------- */
static void flash_wait_busy(void)
{
    while (FLASH->SR & (FLASH_SR_BSY1 | FLASH_SR_BSY2)) { }
}

/* -------------------------------------------------------------------------
 * flash_erase_bank2_page()
 *
 * Erases one 2 KB page in flash bank 2. All bytes become 0xFF after erase.
 * Flash must be unlocked and the device must be in dual-bank mode.
 * @page: page index within bank 2 (0–63 on the 256 KB STM32G0C1xC)
 * ------------------------------------------------------------------------- */
static void flash_erase_bank2_page(uint32_t page)
{
    flash_wait_busy();

    /* Clear any previous error flags */
    FLASH->SR = FLASH->SR;

    /* Page erase in bank 2: PER + BKER, PNB = 256 + page, then start */
    FLASH->CR = FLASH_CR_PER
              | FLASH_CR_BKER
              | (((FLASH_BANK2_PNB_BASE + page) << FLASH_CR_PNB_Pos) & FLASH_CR_PNB_Msk)
              | FLASH_CR_STRT;

    flash_wait_busy();

    /* Clear erase-mode bits */
    FLASH->CR &= ~(FLASH_CR_PER | FLASH_CR_BKER | FLASH_CR_PNB | FLASH_CR_STRT);
}

/* -------------------------------------------------------------------------
 * flash_write_dword()
 *
 * Writes a 64-bit double-word (two consecutive 32-bit words) to flash.
 * Flash must be unlocked and the target page must be erased (all 0xFF).
 * The target address must be 8-byte aligned.
 *
 * STM32G0C1 requirement: both 32-bit words of a double-word must be
 * written before the programming latch fires. Writing only one word
 * leaves the page in a partially-programmed state.
 *
 * @addr: destination address (must be 8-byte aligned, in flash range)
 * @lo:   lower 32 bits (written first)
 * @hi:   upper 32 bits (written second, triggers latch)
 * ------------------------------------------------------------------------- */
static void flash_write_dword(uint32_t addr, uint32_t lo, uint32_t hi)
{
    flash_wait_busy();

    FLASH->CR |= FLASH_CR_PG;   /* Enable programming mode */

    /* Write lower word first, then upper — order matters */
    *(__IO uint32_t *)(addr)     = lo;
    *(__IO uint32_t *)(addr + 4) = hi;

    flash_wait_busy();

    FLASH->CR &= ~FLASH_CR_PG;  /* Clear programming mode */
}

/* -------------------------------------------------------------------------
 * flash_save()
 *
 * Saves current g_kp_q16, g_ki_q16, and g_target_period to flash.
 *
 * Procedure:
 *   1. Build settings block in SRAM with current values + CRC
 *   2. Unlock flash
 *   3. Erase settings page
 *   4. Write block as double-words (32 bytes = 4 double-words)
 *   5. Lock flash
 *   6. Verify readback
 *
 * Safe to call from main loop only — not from ISR context.
 *
 * Missed FG edge mitigation (Option 2):
 *   The settings page is in bank 2 while the firmware (currently well
 *   under 128 KB) executes from bank 1, so in dual-bank mode the erase
 *   (~2ms) and double-word programs (~85µs each) should not stall
 *   instruction fetch. The freeze below is kept anyway: it costs nothing
 *   measurable and stays correct if the image ever grows into bank 2, in
 *   which case fetches from bank 2 stall and FG edges would be missed.
 *
 *   Before erasing, servo_freeze() is called to:
 *     1. Disable the TIM2 capture interrupt (no more ISR executions)
 *     2. Latch the current DAC output at its last known-good value
 *   The motor continues running at constant drive during the write.
 *   After flash_lock(), servo_unfreeze() re-enables the TIM2 interrupt
 *   and resets the integral to zero, giving the loop a clean start.
 *
 *   Motor speed change during a ~2ms blackout is negligible — the WM-D6C
 *   flywheel time constant is ~200ms. Speed deviation is unmeasurable.
 *
 * Returns: FLASH_OK on success, FLASH_ERR_VERIFY on readback mismatch,
 *          FLASH_ERR_BANK_MODE if the device is not in dual-bank mode.
 * ------------------------------------------------------------------------- */
FlashResult flash_save(void)
{
    SettingsBlock blk;

    /* Build block in SRAM before touching flash */
    blk.magic         = SETTINGS_MAGIC;
    blk.kp_q16        = g_kp_q16;
    blk.ki_q16        = g_ki_q16;
    blk.target_period = g_target_period;
    blk.reserved[0]   = 0xFFFFFFFFUL;
    blk.reserved[1]   = 0xFFFFFFFFUL;
    blk.reserved[2]   = 0xFFFFFFFFUL;

    /* CRC covers magic + kp + ki + target (16 bytes = first 4 fields) */
    blk.crc32 = crc32_compute((const uint8_t *)&blk, offsetof(SettingsBlock, crc32));

    /* The bank-2 page addressing below is only valid in dual-bank mode. */
    if (!(FLASH->OPTR & FLASH_OPTR_DUAL_BANK)) {
        return FLASH_ERR_BANK_MODE;
    }

    /* Freeze servo: latch DAC, disable TIM2 ISR before flash stall window */
    servo_freeze();

    flash_unlock();
    flash_erase_bank2_page(SETTINGS_BANK2_PAGE);

    /* Write 32 bytes as 4 double-words (8 bytes each) */
    const uint32_t *src  = (const uint32_t *)&blk;
    uint32_t        addr = FLASH_SETTINGS_ADDR;

    for (int i = 0; i < 4; i++) {
        flash_write_dword(addr, src[i*2], src[i*2 + 1]);
        addr += 8;
    }

    flash_lock();

    /* Unfreeze servo: reset integral, re-enable TIM2 ISR */
    servo_unfreeze();

    /* Verify readback */
    const SettingsBlock *stored = (const SettingsBlock *)FLASH_SETTINGS_ADDR;
    if (stored->magic         != blk.magic         ||
        stored->kp_q16        != blk.kp_q16        ||
        stored->ki_q16        != blk.ki_q16        ||
        stored->target_period != blk.target_period ||
        stored->crc32         != blk.crc32) {
        return FLASH_ERR_VERIFY;
    }

    return FLASH_OK;
}

/* -------------------------------------------------------------------------
 * flash_load()
 *
 * Reads the settings block from flash and, if valid, applies the saved
 * constants to g_kp_q16, g_ki_q16, and g_target_period.
 *
 * Validation:
 *   1. Magic value must equal SETTINGS_MAGIC
 *   2. CRC-32 of the first 16 bytes must match the stored crc32 field
 *   3. Gain constants must be within the safe ranges defined in config.h
 *
 * If any check fails, compiled-in defaults are used and no flash write
 * is performed. The caller (main.c) may choose to call flash_save() to
 * overwrite a corrupt block with defaults.
 *
 * Safe to call from main() before interrupts are enabled.
 *
 * Returns: FLASH_OK if saved settings loaded, FLASH_ERR_INVALID if defaults used.
 * ------------------------------------------------------------------------- */
FlashResult flash_load(void)
{
    const SettingsBlock *stored = (const SettingsBlock *)FLASH_SETTINGS_ADDR;

    /* Check magic */
    if (stored->magic != SETTINGS_MAGIC) {
        return FLASH_ERR_INVALID;
    }

    /* Check CRC */
    uint32_t expected_crc = crc32_compute(
        (const uint8_t *)stored,
        offsetof(SettingsBlock, crc32)
    );
    if (stored->crc32 != expected_crc) {
        return FLASH_ERR_INVALID;
    }

    /* Range-check gain constants — reject values outside safe operating range.
     * A corrupted flash page could pass CRC by chance if only a few bits
     * flipped in non-CRC-covered fields; range checking adds a second layer. */
    if (stored->kp_q16 < KP_Q16_MIN || stored->kp_q16 > KP_Q16_MAX) {
        return FLASH_ERR_INVALID;
    }
    if (stored->ki_q16 < KI_Q16_MIN || stored->ki_q16 > KI_Q16_MAX) {
        return FLASH_ERR_INVALID;
    }
    if (stored->target_period < ADJUSTED_TARGET_MIN ||
        stored->target_period > ADJUSTED_TARGET_MAX) {
        return FLASH_ERR_INVALID;
    }

    /* All checks passed — apply saved constants */
    g_kp_q16        = stored->kp_q16;
    g_ki_q16        = stored->ki_q16;
    g_target_period = stored->target_period;

    return FLASH_OK;
}

/* -------------------------------------------------------------------------
 * flash_reset_defaults()
 *
 * Restores compiled-in defaults to SRAM without writing flash.
 * The 'r' USB CDC command calls this. The user must then send 's' to
 * persist the defaults to flash — this is intentional, preventing an
 * accidental reset from permanently overwriting calibrated constants.
 * ------------------------------------------------------------------------- */
void flash_reset_defaults(void)
{
    g_kp_q16        = KP_Q16_DEFAULT;
    g_ki_q16        = KI_Q16_DEFAULT;
    g_target_period = TARGET_PERIOD_DEFAULT;
}
