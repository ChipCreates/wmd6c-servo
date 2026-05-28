
# Referenced Datasheets — wmd6c-servo DSR-1

All datasheets listed here are copyright their respective manufacturers.
No PDF files are stored in this repository. Direct links to manufacturer
sources are provided for reference and download.

---

## Primary MCU

| Document | Manufacturer | Doc Number | Link |
|---|---|---|---|
| STM32G0B1xB/xC/xE Datasheet | STMicroelectronics | DS13560 Rev 6 | https://www.st.com/resource/en/datasheet/stm32g0b1cc.pdf |
| STM32G0x1 Reference Manual | STMicroelectronics | RM0444 Rev 6 | https://www.st.com/resource/en/reference_manual/rm0444-stm32g0x1-advanced-armbased-32bit-mcus-stmicroelectronics.pdf |
| STM32G0B1KBU6 Product Page | STMicroelectronics | — | https://www.st.com/en/microcontrollers-microprocessors/stm32g0b1re.html |

---

## Power Management

| Document | Manufacturer | Doc Number | Link |
|---|---|---|---|
| MT3608 Boost Converter | XI'AN Aerosemi Tech | — | https://www.lcsc.com/datasheet/lcsc_datasheet_1811151539_XI-AN-Aerosemi-Tech-MT3608_C84817.pdf |
| MCP1700 LDO Regulator | Microchip Technology | DS20001826F | https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP1700-Data-Sheet-20001826F.pdf |

---

## Variant A — USB-C PD Components

| Document | Manufacturer | Doc Number | Link |
|---|---|---|---|
| IP2721 USB PD Sink Trigger | INJOINIC | — | https://www.lcsc.com/datasheet/lcsc_datasheet_2006111335_INJOINIC-IP2721_C603176.pdf |
| USBLC6-2SC6Y ESD Protection | STMicroelectronics | — | https://www.st.com/resource/en/datasheet/usblc6-2sc6y.pdf |

---

## Variant B — Barrel Jack Protection Components

| Document | Manufacturer | Doc Number | Link |
|---|---|---|---|
| LTC4359 Ideal Diode Controller | Analog Devices | — | https://www.analog.com/media/en/technical-documentation/data-sheets/ltc4359.pdf |

---

## Discrete Semiconductors

| Document | Manufacturer | Package | Link |
|---|---|---|---|
| BAT54 Schottky Diode | Vishay | SOD-123 | https://www.vishay.com/docs/85508/bat54.pdf |
| SS14 Schottky Rectifier | Vishay | SMA | https://www.vishay.com/docs/88754/ss14.pdf |
| SMBJ7.0A TVS Diode | Vishay | SMA | https://www.vishay.com/docs/88358/smbj.pdf |
| MMBT3904 NPN Transistor | onsemi | SOT-23 | https://www.onsemi.com/pdf/datasheet/mmbt3904-d.pdf |

---

## WM-D6C Machine Components (Reference Only)

| Document | Notes |
|---|---|
| Sony CX20084 Servo IC | Proprietary Sony ASIC — no public datasheet available |
| Sharp GP2S22AB (FG901) | Search: "GP2S22AB datasheet" on Sharp's component portal at https://www.sharpsma.com |
| 2SB1013 PNP Transistor (Q601, Ver. 1.0) | Search: "2SB1013 datasheet" — multiple sources available |
| MMBT3904 NPN Transistor (Q_LS, DSR-1 motor drive) | Already listed above under Discrete Semiconductors |
| Sony WM-D6C / TC-D6C Service Manual, Ver. 1.1 (2001.06) | `sony_wm-d6c_tc-d6c_ver-1.1.pdf` — present in repo (not committed per .gitignore). Governing reference for Rev A design. Documents the servo circuit change included in Ver. 1.1. |
| Sony WM-D6C Service Manual, earlier edition | `fb4872.pdf` — present in repo. Pre-dates the Ver. 1.1 servo circuit change. Use alongside Ver. 1.1 to identify exactly what changed in the servo circuit and to determine which revision serial 72795 contains. |

---

## License and Attribution Notes

All linked documents are subject to their respective manufacturer's copyright and
terms of use. Links are provided solely for the convenience of engineers working
with this open source project. No endorsement by any manufacturer is implied. 
