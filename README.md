# multizone-sdk
MultiZone® Security for RISC-V processors

**MultiZone® Security** is the quick and safe way to add security and separation to RISC-V processors. MultiZone software can retrofit existing designs. If you don’t have TrustZone-like hardware, or if you require finer granularity than one secure world, you can take advantage of high security separation without the need for hardware and software redesign, eliminating the complexity associated with managing a hybrid hardware/software security scheme. RISC-V standard ISA does't include TrustZone-like primitives to provide the necessary separation, thus leading to larger attack surface and increased likelihood of vulnerability. To shield critical functionality from untrusted third-party components, MultiZone provides hardware-enforced, software-defined separation of multiple equally secure worlds. Unlike antiquated hypervisor-like solutions, MultiZone is self-contained, presents an extremely small attack surface, and it is policy driven, meaning that no coding is required – and in fact even allowed.

MultiZone works with any RISC-V processors with PMP and U mode (rv32, rv32e, rv64, ICU).

This version of the GNU-based SDK supports the following development hardware:

 - Hex Five X300 - RV32ACIMU Board: Digilent ARTY7 FPGA
 - UCB E300 (Rocket) - RV32ACIMU Board: Digilent ARTY7 FPGA
 - SiFive E31 - RV32ACIMU Board: Digilent ARTY7 FPGA
 - SiFIve S51 - RV64ACIMU Board: Digilent ARTY7 FPGA

WIP ...
