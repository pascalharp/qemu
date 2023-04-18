# QEMU-ASP: AMD Secure Processor Emulator

Welcome to the early stages of the QEMU-ASP (originally QEMU-PSP) project. This implementation is a continuation of Robert Buhren's [initial version](https://github.com/RobertBuhren/qemu/tree/pspemu), further expanding and refining the emulation capabilities.  It uses [PSPEmu](https://github.com/PSPReverse/PSPEmu) as a reference and aims to provide a basic emulation of the AMD Secure Processor (ASP) using the QEMU framework. The project is in its infancy, but the goal is to create a tool that can eventually help developers and researchers better understand and explore the intricacies of the AMD Secure Processor.

## Current Features
 - Successfully executes the entire on-chip bootloader for multiple Zen version.
 - Support for multiple ZEN generations including Zen, Zen+ and Zen2.
 - Leverages the QEMU ecosystem for powerful inspection and debugging capabilities within the emulation environment

## Building the emulator
```bash
mkdir build
cd build
../configure --target-list=arm-softmmu --enable-debug --disable-tools --disable-guest-agent --disable-virtfs --enable-nettle
make
```

## Running the emulation
After building the emulator the executable `qemu-system-arm` is located in the build directory.
To run the emulation provide the machine (zen version) the path to the on-chip bootloader and the path to the off-chip bios image:
```bash
path/to/build/dir/qemu-system-arm \
    --machine amd-psp_zen \
    -device loader,file=$PATH_TO_ON_CHIP_BL,addr=0xffff0000,force-raw=on \
    -global driver=amd_psp.smnflash,property=flash_img,value=$PATH_TO_BIOS_IMAGE
```

### Trace
Most implemented components offer trace output capabilities. To obtain trace information for the status port and the cryptographic co-processor, simply include the following flag:
```
-d "trace:psp_*,trace:cpp_*"
```
All currently supported AMD Secure Processor specific trace options can be listed with:
```
qemu-system-arm -d "trace:help" | grep -E 'psp|ccp'
```

### GDB
To establish a GDB connection and pause the emulation at its start, add the following flags:
```
-s -S
```

### Other capabilities
For additional utilities, such as the QEMU monitor, refer to the official QEMU documentation.
