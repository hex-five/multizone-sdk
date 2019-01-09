# Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved

.PHONY: all
all: $(TARGET)


ASM_SRCS += $(NEWLIB_DIR)/crt0.S
C_SRCS += $(NEWLIB_DIR)/newlib.c
C_SRCS += $(PLATFORM_DIR)/plic_driver.c

INCLUDES += -I$(PLATFORM_DIR)

LDFLAGS += -T $(LINKER_SCRIPT)
LDFLAGS += --specs=nano.specs
LDFLAGS += -nostartfiles
LDFLAGS += -Xlinker --gc-sections

ASM_OBJS := $(ASM_SRCS:.S=.o)
C_OBJS := $(C_SRCS:.c=.o)

LINK_OBJS += $(ASM_OBJS) $(C_OBJS)
LINK_DEPS += $(LINKER_SCRIPT)

CLEAN_OBJS += $(TARGET) $(LINK_OBJS)

CFLAGS += -g
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -march=$(RISCV_ARCH)
CFLAGS += -mabi=$(RISCV_ABI)
CFLAGS += -mcmodel=medany
CFLAGS += -msmall-data-limit=8
CFLAGS += -mdiv

HEX = $(subst .elf,.hex,$(TARGET))
RISCV_OBJCOPY := $(dir $(CC))$(subst -gcc,-objcopy,$(notdir $(CC)))
CLEAN_OBJS += $(HEX) 

$(TARGET): $(LINK_OBJS) $(LINK_DEPS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LINK_OBJS) -o $@ $(LDFLAGS)
	$(RISCV_OBJCOPY) -O ihex $(TARGET) $(HEX) --gap-fill 0x00

$(ASM_OBJS): %.o: %.S $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(C_OBJS): %.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(CLEAN_OBJS) 

