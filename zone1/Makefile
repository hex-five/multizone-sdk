# Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved

TARGET = zone1.elf

CFLAGS += -O0 -g3 -ffunction-sections -fdata-sections 
LDFLAGS += -Xlinker --gc-sections -static --specs=nosys.specs 

BSP_BASE := ../bsp
PLATFORM_DIR := $(BSP_BASE)/$(BOARD)
NEWLIB_DIR := $(PLATFORM_DIR)/newlib

LINKER_SCRIPT := flash.lds

C_SRCS += main.c
HEADERS += FreeRTOSConfig.h

FREERTOS_SRC_DIR := ./Source
C_SRCS += \
	$(FREERTOS_SRC_DIR)/croutine.c \
	$(FREERTOS_SRC_DIR)/event_groups.c \
	$(FREERTOS_SRC_DIR)/list.c \
	$(FREERTOS_SRC_DIR)/queue.c \
	$(FREERTOS_SRC_DIR)/stream_buffer.c \
	$(FREERTOS_SRC_DIR)/tasks.c \
	$(FREERTOS_SRC_DIR)/timers.c \
	$(FREERTOS_SRC_DIR)/portable/MemMang/heap_4.c \

C_SRCS += $(FREERTOS_SRC_DIR)/portable/GCC/RISCV/port.c
ASM_SRCS += $(FREERTOS_SRC_DIR)/portable/GCC/RISCV/portasm.S

INCLUDES = \
	-I. \
	-I$(FREERTOS_SRC_DIR)/include \
	-I$(FREERTOS_SRC_DIR)/portable/GCC/RISCV 


INCLUDES += -I../multizone/libhexfive
LDFLAGS += -L../multizone/libhexfive
LDFLAGS += $(if $(findstring rv64, $(RISCV_ARCH)), -lhexfive64, -lhexfive32)

include $(NEWLIB_DIR)/newlib.mk

