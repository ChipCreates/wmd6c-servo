# USB PMA Port — STM32G0 Second-Generation USB IP

**Project:** DSR-1 / wmd6c-servo
**File:** `docs/hardware/usb-pma-stm32g0-port.md`
**Status:** Defect note + porting guidance / pre-Rev A
**Scope:** Why the current `usb_cdc.c` packet-memory-area (PMA) access is wrong for
the STM32G0x1 USB device IP used by the selected STM32G0C1KCU6 target, what differs
between the two ST USB device IP generations, and the concrete changes required.
Register names and offsets below must be confirmed against RM0444 before relying on
this note.

---

## 1. Problem statement

`firmware/src/usb_cdc.c` accesses the USB packet memory area and buffer descriptor
table using the **first-generation** ST USB device convention. The selected
STM32G0C1KCU6 target uses the **second-generation** STM32G0x1 device IP, which
accesses the PMA differently. The code
will not enumerate as written.

The two affected constructs:

```c
/* ×2 stride — first-generation "doubled" PMA access */
static inline void pma_write16(uint32_t pma_off, uint16_t val) {
    *(__IO uint16_t *)(USB_PMA_BASE + pma_off * 2) = val;
}

/* Gap halfword after every real field — first-generation descriptor layout */
typedef struct {
    volatile uint16_t ADDR_TX;  uint16_t _pad0;
    volatile uint16_t COUNT_TX; uint16_t _pad1;
    volatile uint16_t ADDR_RX;  uint16_t _pad2;
    volatile uint16_t COUNT_RX; uint16_t _pad3;
} PMA_BufDesc;
```

---

## 2. The two USB IP generations

ST has shipped two generations of the (non-OTG) USB device peripheral. They differ
in how the CPU sees the PMA:

| Aspect | First generation | Second generation |
|---|---|---|
| Example parts | STM32F102/F103, L0, F0, F3, L4x2/x3, L5, **G4**, WB55 | **STM32G0**, C0, U0, U535/545, H503/23/33/63/73 |
| PMA access from CPU | 16-bit words mapped onto 32-bit boundaries ("2×16-bit/word"): a gap word sits between each real 16-bit word | Linear 1:1: 16-bit words are contiguous in the address space |
| Descriptor table stride | Doubled (each 16-bit field occupies 4 bytes of CPU address space) | Native (each 16-bit field occupies 2 bytes) |
| Effect on this code | Matches the `×2` stride and `_pad` words | Both the `×2` stride and the `_pad` words are wrong |

Practical consequence: on the G0, a 16-bit PMA word at PMA offset `N` (bytes) lives
at `USB_SRAM_BASE + N`, and the buffer descriptor entries are packed contiguously
with no padding. The current code writes to the wrong addresses and leaves
unwritten gaps the hardware does not expect.

> This is a documented IP-generation difference, not a board-specific quirk. It is
> the single reason the present USB layer cannot work on the selected STM32G0C1KCU6 target, independent
> of any descriptor or endpoint-state-machine issues.

---

## 3. Required changes

### 3.1 PMA base and access

Confirm the USB SRAM base for the G0 device IP from RM0444 (the legacy
`0x40006000` F1 base does not apply). Then drop the stride doubling:

```c
/* STM32G0: PMA is accessed linearly (1:1). Verify USB_SRAM_BASE in RM0444. */
#define USB_SRAM_BASE   0x40009800UL   /* <-- confirm against RM0444 */

static inline void pma_write16(uint32_t pma_off, uint16_t val) {
    *(__IO uint16_t *)(USB_SRAM_BASE + pma_off) = val;   /* no ×2 */
}
static inline uint16_t pma_read16(uint32_t pma_off) {
    return *(__IO uint16_t *)(USB_SRAM_BASE + pma_off);  /* no ×2 */
}
```

`pma_write_buf()` / `pma_read_buf()` keep their halfword-packing logic, but the
inner `pma_write16(pma_off + i*2, …)` offset arithmetic is now in real bytes, which
is what you want — just verify the `+ i*2` still indexes contiguous halfwords once
the stride doubling is removed.

### 3.2 Buffer descriptor table

Remove the gap words; the second-generation descriptor entries are contiguous
16-bit values:

```c
/* STM32G0: contiguous 16-bit descriptor fields, no padding. */
typedef struct __attribute__((packed)) {
    volatile uint16_t ADDR_TX;
    volatile uint16_t COUNT_TX;
    volatile uint16_t ADDR_RX;
    volatile uint16_t COUNT_RX;
} PMA_BufDesc;
```

The `COUNT_RX` field encodes the allocated buffer size via the block-size
(`BLSIZE`) and number-of-blocks (`NUM_BLOCK`) fields. Confirm the exact bit layout
in RM0444 — for a 64-byte endpoint it is typically `BLSIZE=1` (32-byte blocks) with
`NUM_BLOCK=1`, but verify rather than copy a first-generation value.

### 3.3 Endpoint / channel registers (verify together)

The second-generation IP also renames and reworks the per-endpoint registers
(channel/endpoint registers and their toggle-bit semantics) relative to the classic
`EPnR`. Before declaring USB done, check against RM0444:

- the per-endpoint register name and the read-modify-write toggle/clear semantics
  (the "write-1-to-toggle, write-0-to-leave" behavior is easy to get subtly wrong),
- the `BTABLE` location/programming,
- the control register and interrupt-status register field names,
- whether any USB SRAM clock/isolation or transceiver-enable bit must be set that
  the first-generation flow did not require.

### 3.4 Things that do not change

- The descriptor *contents* (device/config/CDC class descriptors) are USB-spec
  level and are unaffected by the PMA model.
- The crystal-less clocking plan (HSI48 + CRS) is independent of PMA access and
  remains valid; see `docs/theory/usb-crystalless-operation.md`.
- The priority-1 USB / priority-0 servo split is unaffected.

---

## 4. Validation

Fold these into `docs/verification/07-usb-c-data-pd-test-plan.md`:

1. Device enumerates as a CDC ACM virtual COM port on Linux, macOS, and Windows
   with no driver install.
2. The boot banner is received intact on first connection (exercises an EP1-IN
   transfer through the rewritten PMA path).
3. A host→device command (`t`) returns a correctly framed telemetry line
   (exercises EP2-OUT and the RX descriptor).
4. Sustained continuous telemetry (`T`) at the configured interval shows no
   dropped or corrupted lines (exercises descriptor turnaround and COUNT fields).
5. With telemetry streaming, confirm on the scope that the servo timing on the
   FG-derived PWM output is undisturbed (priority hierarchy intact).

---

## 5. Open question

Decide whether to keep a hand-written register-level USB stack or adopt a small
vetted bare-metal CDC implementation already validated on the STM32G0 second-gen IP.
The project's "register-level, RM0444-authoritative" rule favors keeping it
in-house, but the PMA and endpoint-register rewrite is exactly the area where a
known-good reference saves the most bring-up time. Either way, the result must be
register-level and HAL-free per the contributor rule.
