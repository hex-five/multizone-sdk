# Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved

.PHONY: all
all: $(TARGET)

ASM_SRCS += $(NEWLIB_DIR)/crt0.S
C_SRCS += $(NEWLIB_DIR)/newlib.c

INCLUDES += -I$(PLATFORM_DIR)

LDFLAGS += -T $(PLATFORM_DIR)/memory.lds
LDFLAGS += -T $(LINKER_SCRIPT)
LDFLAGS += --specs=nano.specs
LDFLAGS += --specs=nosys.specs
LDFLAGS += -nostartfiles
LDFLAGS += -Xlinker --gc-sections
LDFLAGS += -Wl,-Map,$(MAP)

ASM_OBJS := $(ASM_SRCS:.S=.o)
C_OBJS := $(C_SRCS:.c=.o)

LINK_OBJS += $(ASM_OBJS) $(C_OBJS)
LINK_DEPS += $(LINKER_SCRIPT)

CLEAN_OBJS += $(TARGET) $(LINK_OBJS)

CFLAGS += -march=$(RISCV_ARCH)
CFLAGS += -mabi=$(RISCV_ABI)
CFLAGS += -mcmodel=medlow
CFLAGS += -msmall-data-limit=8
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -Wall
CFLAGS += -Os -ggdb

HEX = $(subst .elf,.hex,$(TARGET))
LST = $(subst .elf,.lst,$(TARGET))
MAP = $(subst .elf,.map,$(TARGET))
SIZ = $(subst .elf,.siz,$(TARGET))

$(TARGET): $(LINK_OBJS) $(LINK_DEPS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LINK_OBJS) -o $@ $(LDFLAGS)
	$(OBJCOPY) -O ihex $(TARGET) $(HEX) --gap-fill 0x00
	$(OBJDUMP) --all-headers --demangle --disassemble --file-headers --wide -D $(TARGET) > $(LST)
	$(SIZE) --format=sysv $(TARGET) > $(SIZ)

$(ASM_OBJS): %.o: %.S $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(C_OBJS): %.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf $(TARGET) $(LINK_OBJS) $(HEX) $(LST) $(MAP) $(SIZ)

