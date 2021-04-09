# multizone-sdk
MultiZone® Security TEE for RISC-V processors

**MultiZone® Security** is the quick and safe way to add security and separation to RISC-V processors. MultiZone software can retrofit existing designs. If you don’t have TrustZone-like hardware, or if you require finer granularity than one secure world, you can take advantage of high security separation without the need for hardware and software redesign, eliminating the complexity associated with managing a hybrid hardware/software security scheme. RISC-V standard ISA doesn't define TrustZone-like primitives to provide hardware separation. To shield critical functionality from untrusted third-party components, MultiZone provides hardware-enforced, software-defined separation of multiple equally secure worlds. Unlike antiquated hypervisor-like solutions, MultiZone is self-contained, presents an extremely small attack surface, and it is policy driven, meaning that no coding is required – and in fact even allowed.

MultiZone works with any 32-bit or 64-bit RISC-V standard processors  with Physical Memory Protection unit and U mode.

This version of the GNU-based SDK supports the following hardware:
- [Digilent Arty A7 Development Board (Xilinx Artix-7 FPGA)](https://www.xilinx.com/products/boards-and-kits/arty.html)
- [SiFive HiFive1 Rev B (Freedom E310 SoC)](https://www.sifive.com/boards/hifive1-rev-b)
- [Microchip Icicle Kit (PolarFire SoC)](https://www.microsemi.com/existing-parts/parts/152514)

Note: Microchip Icicle Kit see [https://github.com/hex-five/multizone-sdk-pfsc](https://github.com/hex-five/multizone-sdk-pfsc) 

The Arty FPGA Evaluation Kit works with the following softcore bitstreams:

- [Hex Five X300 RV32ACIMU - Permissive open source. No license required.](https://github.com/hex-five/multizone-fpga)
- [SiFive E31 RV32ACIMU - Proprietary. SiFive license required.](https://www.sifive.com/cores/e31)
- [SiFive S51 RV64ACIMU - Proprietary. SiFive license required.](https://www.sifive.com/cores/s51)

The Xilinx Arty FPGA comes in two versions: 35T and 100T

- Hex Five's X300 bitstream works with version 35T
- SiFive's bitstreams up to v19.02 work with version 35T
- SiFive's bitstreams after v19.02 work with version 100T

For instructions on how to upload the bitstream to the ARTY board and how to connect the [Olimex debug head ARM-USB-TINY-H](https://www.olimex.com/Products/ARM/JTAG/ARM-USB-TINY-H/) see [Arty FPGA Dev Kit Getting Started Guide](https://sifive.cdn.prismic.io/sifive%2Fed96de35-065f-474c-a432-9f6a364af9c8_sifive-e310-arty-gettingstarted-v1.0.6.pdf)


### MultiZone SDK Installation ###

The MultiZone SDK works with any versions of Linux, Windows, and Mac capable of running Java 1.8 or greater. The directions in this readme have been verified with fresh installations of Ubuntu 20.04, Ubuntu 19.10, Ubuntu 18.04.5, and Debian 10.5. Other Linux distros are similar. Windows developers may want to install a Linux emulation environment like Cygwin or run the SDK in a Linux VM guest (2GB Disk, 2GB Ram)

**Linux prerequisites**

```
sudo apt update
sudo apt install make default-jre gtkterm libhidapi-dev libftdi1-2
```
Ubuntu 18.04 LTS additional dependency

```
sudo add-apt-repository "deb http://archive.ubuntu.com/ubuntu/ focal main universe"
sudo apt update
sudo apt install libncurses-dev
```
Note: GtkTerm is optional and required only to connect to the reference application via UART. It is not required to build, debug, and load the MultiZone software. Any other serial terminal application of choice would do.

**GNU RISC-V Toolchain**

Hex Five reference build: RISC-V GNU Toolchain Linux 64-bit June 13, 2020
```
cd ~
wget https://hex-five.com/wp-content/uploads/riscv-gnu-toolchain-20200613.tar.xz
tar -xvf riscv-gnu-toolchain-20200613.tar.xz
```

**OpenOCD on-chip debugger**

Hex Five reference build: RISC-V openocd Linux 64-bit June 13, 2020
```
cd ~
wget https://hex-five.com/wp-content/uploads/riscv-openocd-20200613.tar.xz
tar -xvf riscv-openocd-20200613.tar.xz
```
Note: the SiFive HiFive1 board doesn't support OpenOCD and requires the Segger propietary package JLink_Linux_V694_x86_64.deb downloadable at [https://www.segger.com/downloads/jlink/](https://www.segger.com/downloads/jlink/). 

**Linux USB udev rules**

```
sudo vi /etc/udev/rules.d/99-openocd.rules

# Future Technology Devices International, Ltd FT2232C Dual USB-UART/FIFO IC
SUBSYSTEM=="tty", ATTRS{idVendor}=="0403",ATTRS{idProduct}=="6010", MODE="664", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor} =="0403",ATTR{idProduct} =="6010", MODE="664", GROUP="plugdev"

# Olimex Ltd. ARM-USB-TINY-H JTAG interface
SUBSYSTEM=="tty", ATTRS{idVendor}=="15ba",ATTRS{idProduct}=="002a", MODE="664", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor} =="15ba",ATTR{idProduct} =="002a", MODE="664", GROUP="plugdev"

# SiFive HiFive1 Rev B00 - SEGGER
SUBSYSTEM=="tty", ATTRS{idVendor}=="1366",ATTRS{idProduct}=="1051", MODE="664", GROUP="plugdev"
```
Reboot for these changes to take effect.

**MultiZone Security SDK**

```
cd ~
wget https://github.com/hex-five/multizone-sdk/archive/master.zip
unzip master.zip
mv multizone-sdk-master multizone-sdk
```

### Build & load the MultiZone reference application ###

Connect the target board to the development workstation as indicated in the user manual.

'ls multizone-sdk/bsp' shows the list of supported targets: X300, FE310, E31, S51, PFSOC.

Assign one of these values to the BOARD variable - default is X300.

```
cd ~/multizone-sdk
export RISCV=~/riscv-gnu-toolchain-20200613
export OPENOCD=~/riscv-openocd-20200613
export BOARD=X300
make 
make load
```
Note: With some older versions of the ftdi libraries, the first "make load" after powering the board may take a bit longer than it should. If you don't want to wait, the simple workaround is to reset the FPGA board to abort the openOCD session. If you do this, make sure to kill the openocd process on your computer. Subsequent loads will work as expected and take approximately 10 seconds.

Important: make sure that switch SW3 is positioned close to the edge of the board.

Important: open jumper JP2 (CK RST) to prevent a system reset upon UART connection.



### Run the MultiZone reference application ###

Connect the UART port (ARTY micro USB J10) as indicated in the user manual.

On your computer, start a serial terminal console (GtkTerm) and connect to /dev/ttyUSB1 at 115200-8-N-1

Hit the enter key a few times until the cursor 'Z1 >' appears on the screen

Enter 'restart' to display the splash screen

Hit enter again to show the list of available commands

```
=====================================================================
                       Hex Five MultiZone® Security                    
    Copyright© 2020 Hex Five Security, Inc. - All Rights Reserved    
=====================================================================
This version of MultiZone® Security is meant for evaluation purposes 
only. As such, use of this software is governed by the Evaluation    
License. There may be other functional limitations as described in   
the evaluation SDK documentation. The commercial version of the      
software does not have these restrictions.                           
=====================================================================
Machine ISA   : 0x40101105 RV32 ACIMU 
Vendor        : 0x0000057c Hex Five, Inc. 
Architecture  : 0x00000001 X300 
Implementation: 0x20181004 
Hart id       : 0x0 
CPU clock     : 64 MHz 
RTC clock     : 16 KHz 

Z1 > Commands: yield send recv pmp load store exec dma stats timer restart 

Z1 > 
```

### Technical Specs ###
| |
|---|
| Up to 8 hardware threads (zones) hardware-enforced, software-defined |
| Up to 8 memory mapped resources per zone – i.e. flash, ram, rom, i/o, etc. |
| Scheduler: preemptive, cooperative, round robin, configurable tick |
| Secure interzone communications based on messages – no shared memory |
| Built-in support for secure shared Timer interrupt |
| Built-in support for secure shared PLIC interrupt |
| Built-in support for secure DMA transfers |
| Built-in trap & emulation for all privileged instructions – CSRR, CSRW, WFI, etc. |
| Support for secure user-mode interrupt handlers mapped to zones – up to 32 sources PLIC / CLIC |
| Support for Wait For Interrupt and CPU suspend mode for low power applications |
| Formally verifiable runtime ~2KB, 100% written in assembly, no 3rd-party dependencies |
| C library wrapper for protected mode execution – optional for high speed / low-latency |
| Hardware requirements: RV32, RV32e, RV64 cpu with Memory Protection Unit and U extension | 
| System requirements: 6KB FLASH, 4KB RAM - CPU overhead < 0.01% | 
| Development environment: any versions of Linux, Windows, Mac running Java 1.8 |


### Additional Resources ###

- [MultiZone Reference Manual](http://github.com/hex-five/multizone-sdk/blob/master/manual.pdf)
- [MultiZone Datasheet](https://hex-five.com/wp-content/uploads/2020/01/multizone-datasheet-20200109.pdf)
- [MultiZone Website](https://hex-five.com/multizone-security-sdk/)
- [Frequently Asked Questions](http://hex-five.com/faq/)
- [Contact Hex Five http://hex-five.com/contact](http://hex-five.com/contact)


### Legalities ###

Please remember that export/import and/or use of strong cryptography software, providing cryptography hooks, or even just communicating technical details about cryptography software is illegal in some parts of the world. So when you import this software to your country, re-distribute it from there or even just email technical suggestions or even source patches to the authors or other people you are strongly advised to pay close attention to any laws or regulations which apply to you. Hex Five Security, Inc. and the authors of the software included in this repository are not liable for any violations you make here. So be careful, it is your responsibility.

MultiZone and HEX-Five are registered trademarks of Hex Five Security, Inc.

MultiZone technology is patent pending US 16450826, PCT US1938774.


