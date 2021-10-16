# Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved

TARGET = zone3.elf

BSP_BASE := ../bsp
PLATFORM_DIR := $(BSP_BASE)/$(BOARD)
NEWLIB_DIR := $(PLATFORM_DIR)/newlib

LINKER_SCRIPT := linker.lds

C_SRCS += main.c owi_sequence.c

FREERTOS_SRC_DIR := ../ext/freertos
C_SRCS += $(FREERTOS_SRC_DIR)/list.c
C_SRCS += $(FREERTOS_SRC_DIR)/tasks.c
C_SRCS += $(FREERTOS_SRC_DIR)/portable/MemMang/heap_4.c
C_SRCS += $(FREERTOS_SRC_DIR)/portable/GCC/RISC-V/port.c
ASM_SRCS += $(FREERTOS_SRC_DIR)/portable/GCC/RISC-V/portASM.S

INCLUDES += -I ../
INCLUDES += -I ../ext

INCLUDES += -I$(FREERTOS_SRC_DIR)/include
INCLUDES += -I$(FREERTOS_SRC_DIR)/portable/GCC/RISC-V
INCLUDES += -I$(FREERTOS_SRC_DIR)/portable/GCC/RISC-V/chip_specific_extensions/RV32I_CLINT_no_extensions

include $(NEWLIB_DIR)/newlib.mk
