# Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved


#############################################################
# Toolchain definitions
#############################################################

ifndef RISCV
$(error RISCV not set)
endif

export CROSS_COMPILE := $(abspath $(RISCV))/bin/riscv64-unknown-elf-
export CC      := $(CROSS_COMPILE)gcc
export OBJDUMP := $(CROSS_COMPILE)objdump
export OBJCOPY := $(CROSS_COMPILE)objcopy
export GDB     := $(CROSS_COMPILE)gdb
export AR      := $(CROSS_COMPILE)ar


#############################################################
# Platform definitions
#############################################################

BOARD ?= E31
ifeq ($(BOARD),E31)
	RISCV_ARCH := rv32imac
	RISCV_ABI := ilp32
else ifeq ($(BOARD),E51)
	RISCV_ARCH := rv64imac
	RISCV_ABI := lp64
else
	$(error Unsupported board $(BOARD))
endif


#############################################################
# Arguments/variables available to all submakes
#############################################################

export BOARD
export RISCV_ARCH
export RISCV_ABI


#############################################################
# Rules for building multizone
#############################################################

.PHONY: all 
all: 
	$(MAKE) -C zone1
	$(MAKE) -C zone2
	$(MAKE) -C zone3
	$(MAKE) -C multizone

.PHONY: clean
clean: 
	$(MAKE) -C zone1 clean
	$(MAKE) -C zone2 clean
	$(MAKE) -C zone3 clean
	$(MAKE) -C multizone clean


#############################################################
# Load and debug variables and rules
#############################################################

ifndef OPENOCD
$(error OPENOCD not set)
endif

OPENOCD := $(abspath $(OPENOCD))/bin/openocd

OPENOCDCFG ?= bsp/$(BOARD)/openocd.cfg
OPENOCDARGS += -f $(OPENOCDCFG)

GDB_PORT ?= 3333
GDB_LOAD_ARGS ?= --batch
GDB_LOAD_CMDS += -ex "set mem inaccessible-by-default off"
GDB_LOAD_CMDS += -ex "set remotetimeout 240"
GDB_LOAD_CMDS += -ex "target extended-remote localhost:$(GDB_PORT)"
GDB_LOAD_CMDS += -ex "monitor reset halt"
GDB_LOAD_CMDS += -ex "monitor flash protect 0 64 last off"
GDB_LOAD_CMDS += -ex "load"
GDB_LOAD_CMDS += -ex "monitor resume"
GDB_LOAD_CMDS += -ex "monitor shutdown"
GDB_LOAD_CMDS += -ex "quit"

.PHONY: load

load:
	$(OPENOCD) $(OPENOCDARGS) & \
	$(GDB) multizone/multizone.hex $(GDB_LOAD_ARGS) $(GDB_LOAD_CMDS)

