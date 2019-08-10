# Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved

TARGET = zone3.elf

BSP_BASE := ../bsp
PLATFORM_DIR := $(BSP_BASE)/$(BOARD)
NEWLIB_DIR := $(PLATFORM_DIR)/newlib

LINKER_SCRIPT := linker.lds

C_SRCS += main.c owi_task.c

INCLUDES += -I../libhexfive
LDFLAGS += -L../libhexfive
LDFLAGS += $(if $(findstring rv64, $(RISCV_ARCH)), -lhexfive64,  $(if $(findstring rv32e, $(RISCV_ARCH)), -lhexfive32e, -lhexfive32))

include $(NEWLIB_DIR)/newlib.mk

