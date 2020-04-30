# ColoRat09

    Modulary clock-timing based CPU emulator based around real design logic for
a real hardware design in FPGA/CLPD, new, NOS, or used hardware logic chips. If
it can become real hardware, it's good to model/emulate it here.
	
	Initial design is covering the Motorola 6809 MPU. However, it isn't just
limited to the 6809 or even the 6309 MPUs that existed or still exist on the
market. There is nothing that says a 6502, 6510, 6800, 6801, 6802, 6805, 68008,
z80, or some other 8-bit data bus CPU cannot be created to work within the same
framework.

Planned Phases:
    1) Create the CPU (6809 first) model
    2) Create the memory model and MMU
    3) Create the support hardware model

Targets for initial emulation:
    1) Tandy/RadioShack TRS-80 Color Computer 2 (with 6847T1 VDG and 6883 SAM
	   aka: 74ls783, or 74ls785 (enhanced for different DRAM chip refresh
	   timing.) with 4K, 16K, 32K or 64K* of RAM.
	2) Tandy Color Computer 3 with 128K-8MB if RAM (128K, 256K, 512K, 1M, 2M,
	   4M, 8M)
	3) A non-existing PC based off the Motorola 6809 or Hitachi 6309 MPU with
	   between 512K and 8MB of (S)RAM and add a Yamaha V9958 VDP and sound
	   chips. RAM page size of 4K (This may include CoCo 1 and 2, or CoCo 3
	   compatiblility)

Target platform for emulator:
    1) Windows
	2) with assistance, Linux
	3) with assistance, MacOS

























footnotes:
* 64K being the most likely target, skipping the lesser configurations
