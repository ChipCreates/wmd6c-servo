/**
 * flash.h — DSR-1 Settings Flash Storage — Public Interface
 *
 * Include in main.c (calls flash_load on boot) and
 * usb_cdc.c (calls flash_save on 's', flash_reset_defaults on 'r').
 */

#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

/* -------------------------------------------------------------------------
 * Return codes
 * ------------------------------------------------------------------------- */
typedef enum {
    FLASH_OK          = 0,   /* Operation succeeded                          */
    FLASH_ERR_VERIFY  = 1,   /* Readback after write did not match           */
    FLASH_ERR_INVALID = 2,   /* Stored block failed magic / CRC / range check */
} FlashResult;

/* -------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------- */

/**
 * flash_load() — load saved settings from flash into g_kp_q16, g_ki_q16,
 * and g_target_period.
 *
 * Call from main() before enabling interrupts. If the stored block is
 * missing or corrupt, SRAM variables retain their compiled-in defaults
 * from config.h and FLASH_ERR_INVALID is returned.
 */
FlashResult flash_load(void);

/**
 * flash_save() — write current g_kp_q16, g_ki_q16, and g_target_period
 * to flash. Erases the settings page then writes a verified block.
 *
 * Main loop only — not safe to call from ISR context.
 * The servo loop may miss 4–5 FG edges during the erase cycle (~2ms).
 * It recovers automatically within 1–2 FG periods.
 *
 * Returns FLASH_OK on success, FLASH_ERR_VERIFY if readback fails.
 */
FlashResult flash_save(void);

/**
 * flash_reset_defaults() — restore compiled-in defaults to SRAM only.
 * Does NOT write to flash. Call flash_save() afterwards to persist.
 *
 * Triggered by the 'r' USB CDC command.
 */
void flash_reset_defaults(void);

#endif /* FLASH_H */
