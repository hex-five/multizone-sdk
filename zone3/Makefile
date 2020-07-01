# Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved

TARGET = zone3.elf

BSP_BASE := ../bsp
PLATFORM_DIR := $(BSP_BASE)/$(BOARD)
NEWLIB_DIR := $(PLATFORM_DIR)/newlib

LINKER_SCRIPT := linker.lds

C_SRCS += main.c owi_sequence.c

INCLUDES += -I ../

include $(NEWLIB_DIR)/newlib.mk
