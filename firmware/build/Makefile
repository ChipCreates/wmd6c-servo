# =============================================================================
# DSR-1 Firmware Makefile
# WM-D6C Servo Replacement Module
#
# Toolchain: arm-none-eabi-gcc (any version >= 10.0)
# No IDE required. Run from firmware/build/ or set BUILD_DIR.
#
# Usage:
#   make              — build firmware.elf, firmware.bin, firmware.hex
#   make flash        — flash via SWD using OpenOCD (requires openocd installed)
#   make dfu          — flash via USB DFU using dfu-util (device in DFU mode)
#   make clean        — remove all build artefacts
#   make size         — print section sizes
#   make disasm       — generate annotated disassembly (firmware.lss)
#
# Output files (in build/):
#   firmware.elf      — ELF with debug symbols, used for GDB and disassembly
#   firmware.bin      — Raw binary, used for DFU and STM32CubeProgrammer
#   firmware.hex      — Intel HEX, used for some programmers
#   firmware.lss      — Disassembly listing (make disasm only)
#
# Prerequisite check:
#   The Makefile verifies arm-none-eabi-gcc is on PATH and prints a helpful
#   error message if not, rather than a confusing 'command not found'.
# =============================================================================

# -----------------------------------------------------------------------------
# Project name and target device
# -----------------------------------------------------------------------------
PROJECT     := firmware
MCU         := STM32G0B1xx
CPU         := cortex-m0plus
ARCH_FLAGS  := -mcpu=$(CPU) -mthumb

# -----------------------------------------------------------------------------
# Directory layout
#
# This Makefile lives in firmware/build/.
# Source and include files are one level up in firmware/src/ and firmware/include/.
# All build artefacts go into firmware/build/obj/.
# -----------------------------------------------------------------------------
MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
ROOT_DIR     := $(MAKEFILE_DIR)..
SRC_DIR      := $(ROOT_DIR)/src
INC_DIR      := $(ROOT_DIR)/include
BUILD_DIR    := $(MAKEFILE_DIR)
OBJ_DIR      := $(BUILD_DIR)obj

# Startup file (assembly) — lives alongside the C sources
STARTUP      := $(SRC_DIR)/startup_stm32g0b1xx.s

# Linker script — lives in firmware/build/
LDSCRIPT     := $(BUILD_DIR)STM32G0B1KBUx_FLASH.ld

# -----------------------------------------------------------------------------
# OS detection — sets platform-specific shell commands used in targets below.
# GNU make sets $(OS)=Windows_NT on all Windows versions; unset on Unix.
# -----------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    DEVNULL  := NUL
    PYTHON   := python
    WHICH    := where
    MKDIR_OBJ = if not exist "$(subst /,\,$(OBJ_DIR))" mkdir "$(subst /,\,$(OBJ_DIR))"
else
    DEVNULL  := /dev/null
    PYTHON   := python3
    WHICH    := which
    MKDIR_OBJ = mkdir -p $(OBJ_DIR)
endif

# -----------------------------------------------------------------------------
# Toolchain
# -----------------------------------------------------------------------------
CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-gcc   # Use GCC as assembler front-end (handles .s files)
LD      := arm-none-eabi-gcc   # Use GCC as linker (passes -T to ld)
OBJCOPY := arm-none-eabi-objcopy
OBJDUMP := arm-none-eabi-objdump
SIZE    := arm-none-eabi-size

# Verify toolchain is available before doing anything else
ifeq ($(shell $(WHICH) $(CC) 2>$(DEVNULL)),)
$(error arm-none-eabi-gcc not found on PATH. \
  Install it with: \
  macOS:              brew install --cask gcc-arm-embedded \
  Ubuntu/Debian:      sudo apt install gcc-arm-none-eabi \
  Fedora:             sudo dnf install arm-none-eabi-gcc arm-none-eabi-newlib \
  RHEL/CentOS:        sudo dnf install epel-release && sudo dnf install arm-none-eabi-gcc \
  Windows (winget):   winget install Arm.GnuArmEmbeddedToolchain \
  All platforms:      https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
endif

# -----------------------------------------------------------------------------
# Source files
# All .c files in SRC_DIR are compiled. Add new modules here automatically.
# -----------------------------------------------------------------------------
C_SRCS   := $(wildcard $(SRC_DIR)/*.c)
ASM_SRCS := $(STARTUP)

# Object files — mirror source filenames into OBJ_DIR
C_OBJS   := $(patsubst $(SRC_DIR)/%.c,   $(OBJ_DIR)/%.o, $(C_SRCS))
ASM_OBJS := $(patsubst $(SRC_DIR)/%.s,   $(OBJ_DIR)/%.o, $(ASM_SRCS))
ALL_OBJS := $(C_OBJS) $(ASM_OBJS)

# -----------------------------------------------------------------------------
# CMSIS device header search path
#
# The STM32G0B1 CMSIS headers (stm32g0b1xx.h, system_stm32g0xx.h, core_cm0plus.h)
# must be on the include path. Three common locations are checked; set CMSIS_DIR
# to override if your installation is elsewhere.
#
# Common locations:
#   STM32CubeG0 package:  ~/.stm32cube/STM32Cube_FW_G0_V1.x.x/Drivers/CMSIS/
#   ARM CMSIS pack:       ~/STM32PackagesRoot/Keil.STM32G0xx_DFP.1.x.x/
#   System-installed:     /usr/share/stm32cube/g0/
#
# If CMSIS_DIR is not set, we search the three standard locations.
# If none are found, the build will fail with a clear error.
# -----------------------------------------------------------------------------
ifndef CMSIS_DIR
  CMSIS_SEARCH := \
    $(HOME)/.stm32cube/STM32Cube_FW_G0_V1.6.1/Drivers/CMSIS \
    /usr/share/stm32cube/g0/Drivers/CMSIS \
    $(ROOT_DIR)/cmsis

  CMSIS_DIR := $(firstword $(foreach d,$(CMSIS_SEARCH),$(wildcard $(d))))

  ifeq ($(CMSIS_DIR),)
$(warning CMSIS_DIR not set and headers not found in standard locations.)
$(warning Set CMSIS_DIR=/path/to/CMSIS or copy headers to $(ROOT_DIR)/cmsis/)
$(warning See docs/installation/toolchain-setup.md for instructions.)
  endif
endif

# Device-specific and core headers
CMSIS_DEVICE_INC := $(CMSIS_DIR)/Device/ST/STM32G0xx/Include
CMSIS_CORE_INC   := $(CMSIS_DIR)/Include

# -----------------------------------------------------------------------------
# Compiler flags
# -----------------------------------------------------------------------------

# Warning flags — strict but not -Werror (contributors may have older toolchains)
WFLAGS := \
  -Wall \
  -Wextra \
  -Wshadow \
  -Wdouble-promotion \
  -Wundef \
  -fno-common

# C standard and dialect
CSTD := -std=c11

# Optimisation: -O2 as specified in datasheet §6.4
# -ffunction-sections and -fdata-sections enable --gc-sections in linker,
# which removes unused functions and data — important for keeping binary small.
OPT := -O2 -ffunction-sections -fdata-sections

# Debug info — always include, stripped from .bin by objcopy
DBG := -g3 -gdwarf-4

# Include paths
INCLUDES := \
  -I$(INC_DIR) \
  -I$(CMSIS_DEVICE_INC) \
  -I$(CMSIS_CORE_INC)

# Preprocessor defines
DEFINES := \
  -D$(MCU) \
  -DUSE_FULL_LL_DRIVER

# Combined C flags
CFLAGS := $(ARCH_FLAGS) $(CSTD) $(OPT) $(DBG) $(WFLAGS) $(INCLUDES) $(DEFINES)

# Assembler flags (used for startup_stm32g0b1xx.s)
ASFLAGS := $(ARCH_FLAGS) -x assembler-with-cpp $(INCLUDES) $(DEFINES)

# -----------------------------------------------------------------------------
# Linker flags
# -----------------------------------------------------------------------------
LDFLAGS := \
  $(ARCH_FLAGS) \
  -T$(LDSCRIPT) \
  -Wl,--gc-sections \
  -Wl,-Map=$(BUILD_DIR)$(PROJECT).map \
  -Wl,--print-memory-usage \
  -nostdlib \
  -nostartfiles \
  --specs=nano.specs

# nano.specs links against newlib-nano (minimal libc).
# We use it only for memcpy/memset in flash.c and usb_cdc.c.
# No printf, no malloc, no file I/O is used.

# -----------------------------------------------------------------------------
# Build targets
# -----------------------------------------------------------------------------

.PHONY: all flash dfu clean size disasm

all: $(BUILD_DIR)$(PROJECT).bin $(BUILD_DIR)$(PROJECT).hex
	@echo ""
	@echo "Build complete:"
	@$(SIZE) $(BUILD_DIR)$(PROJECT).elf
	@echo ""
	@echo "Flash usage against 128KB device limit (settings sector reserved):"
	-@$(PYTHON) -c "\
import subprocess, re; \
out = subprocess.check_output(['$(SIZE)', '$(BUILD_DIR)$(PROJECT).elf']).decode(); \
m = re.search(r'(\d+)\s+(\d+)\s+(\d+)', out.split('\n')[1]); \
text,data,bss = int(m.group(1)),int(m.group(2)),int(m.group(3)); \
flash=text+data; ram=data+bss; \
print(f'  Flash: {flash} / 126976 bytes ({flash*100//126976}%%) (126KB available, 2KB reserved for settings)'); \
print(f'  RAM:   {ram} / 147456 bytes ({ram*100//147456}%%)')" 2>$(DEVNULL)

# ELF — main build product, contains debug symbols
$(BUILD_DIR)$(PROJECT).elf: $(ALL_OBJS) $(LDSCRIPT)
	@echo "  LD    $@"
	@$(LD) $(LDFLAGS) $(ALL_OBJS) -o $@

# BIN — raw binary for DFU/STM32CubeProgrammer
$(BUILD_DIR)$(PROJECT).bin: $(BUILD_DIR)$(PROJECT).elf
	@echo "  BIN   $@"
	@$(OBJCOPY) -O binary $< $@

# HEX — Intel HEX for some programmers
$(BUILD_DIR)$(PROJECT).hex: $(BUILD_DIR)$(PROJECT).elf
	@echo "  HEX   $@"
	@$(OBJCOPY) -O ihex $< $@

# C source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "  CC    $(notdir $<)"
	@$(CC) $(CFLAGS) -c $< -o $@

# Assembly startup file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s | $(OBJ_DIR)
	@echo "  AS    $(notdir $<)"
	@$(AS) $(ASFLAGS) -c $< -o $@

# Create object directory if it doesn't exist
$(OBJ_DIR):
	@$(MKDIR_OBJ)

# -----------------------------------------------------------------------------
# Flash targets
# -----------------------------------------------------------------------------

# Flash via SWD using OpenOCD (J-Link, ST-Link, CMSIS-DAP all work)
flash: $(BUILD_DIR)$(PROJECT).elf
	openocd \
	  -f interface/stlink.cfg \
	  -f target/stm32g0x.cfg \
	  -c "program $< verify reset exit"

# Flash via USB DFU (device must be in DFU mode: hold BOOT0/TP1, connect USB)
dfu: $(BUILD_DIR)$(PROJECT).bin
	dfu-util \
	  -d 0483:df11 \
	  -a 0 \
	  -s 0x08000000:leave \
	  -D $<

# -----------------------------------------------------------------------------
# Utility targets
# -----------------------------------------------------------------------------

size: $(BUILD_DIR)$(PROJECT).elf
	@$(SIZE) -A $@

disasm: $(BUILD_DIR)$(PROJECT).elf
	@echo "  DISASM $(BUILD_DIR)$(PROJECT).lss"
	@$(OBJDUMP) -h -S $< > $(BUILD_DIR)$(PROJECT).lss
	@echo "  Written to $(BUILD_DIR)$(PROJECT).lss"

ifeq ($(OS),Windows_NT)
clean:
	@echo "  CLEAN"
	-@if exist "$(subst /,\,$(OBJ_DIR))" rmdir /s /q "$(subst /,\,$(OBJ_DIR))"
	-@del /q /f \
	  "$(subst /,\,$(BUILD_DIR)$(PROJECT).elf)" \
	  "$(subst /,\,$(BUILD_DIR)$(PROJECT).bin)" \
	  "$(subst /,\,$(BUILD_DIR)$(PROJECT).hex)" \
	  "$(subst /,\,$(BUILD_DIR)$(PROJECT).map)" \
	  "$(subst /,\,$(BUILD_DIR)$(PROJECT).lss)" 2>NUL
	@echo "  Done"
else
clean:
	@echo "  CLEAN"
	@rm -rf $(OBJ_DIR)
	@rm -f  $(BUILD_DIR)$(PROJECT).elf \
	        $(BUILD_DIR)$(PROJECT).bin \
	        $(BUILD_DIR)$(PROJECT).hex \
	        $(BUILD_DIR)$(PROJECT).map \
	        $(BUILD_DIR)$(PROJECT).lss
	@echo "  Done"
endif

# Print all variables (useful for debugging the build)
print-%:
	@echo '$*=$($*)'
