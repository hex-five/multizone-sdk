# Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved

.PHONY: all
all:
	$(CC) -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -ffreestanding -Wall -x assembler-with-cpp -c -o boot.o boot.S
	$(CC) -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -ffreestanding -Wall -T ../memory.lds -T linker.lds -nostdlib -Xlinker --gc-sections -Wl,-Map,boot.map -o boot.elf ./boot.o 
	$(OBJCOPY) -O ihex -j.boot boot.elf boot.hex
	$(OBJDUMP) --source --all-headers --demangle --disassemble --line-numbers --reloc --wide boot.elf > boot.lst

.PHONY: clean
clean:
	rm -f boot.o boot.hex boot.elf boot.lst boot.map