/*
 * AMD PSP emulation
 *
 * Copyright (C) 2020 Robert Buhren <robert@robertbuhren.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef AMD_PSP_H
#define AMD_PSP_H

#include "hw/arm/boot.h"
#include "hw/arm/psp-fuse.h"
#include "hw/misc/unimp.h"
#include "hw/sysbus.h"
#include "target/arm/cpu.h"
#include "hw/arm/psp-misc.h"
#include "hw/arm/psp-smn.h"
#include "hw/arm/psp-x86.h"
#include "hw/arm/psp-timer.h"
#include "hw/arm/psp-sts.h"
#include "hw/misc/ccpv5.h"

#define TYPE_AMD_PSP "amd-psp"
#define AMD_PSP(obj) OBJECT_CHECK(AmdPspState, (obj), TYPE_AMD_PSP)

#define TYPE_AMD_PSP_ZEN "amd-psp-zen_one"
#define TYPE_AMD_PSP_ZEN_PLUS "amd-psp-zen_plus"
#define TYPE_AMD_PSP_ZEN_TWO "amd-psp-zen_two"

#define PSP_SMN_CTRL_BASE 0x03220000
#define PSP_X86_CTRL1_BASE 0x03230000
#define PSP_X86_CTRL2_BASE 0x032303e0
#define PSP_X86_CTRL3_BASE 0x032304d8

#define PSP_TIMER1_BASE 0x03010400
#define PSP_TIMER2_BASE 0x03010424

#define PSP_STS_ZEN1_BASE 0x032000e8

#define PSP_CCP_BASE 0x03000000

#define PSP_SMN_BASE 0x01000000
#define PSP_SMN_NAME "PSP SMN"

#define PSP_MMIO_BASE 0x03000000
#define PSP_MMIO_NAME "PSP MMIO"

#define PSP_X86_BASE 0x04000000
#define PSP_X86_NAME "PSP X86"

#define PSP_FUSE_BASE 0x03010104
#define PSP_FUSE_NAME "PSP Fuse"

#define PSP_UNKNOWN_BASE  0xfc000000
#define PSP_UNKNOWN_NAME "PSP UNKNOWN"

#define PSP_ROM_BASE  0xffff0000
#define PSP_ROM_NAME "PSP ROM"

#define PSP_ROM_SIZE  0x10000

#define PSP_SRAM_ADDR  0x0
#define PSP_SRAM_SIZE_ZEN  0x40000
#define PSP_SRAM_SIZE_ZEN_PLUS  0x40000
#define PSP_SRAM_SIZE_ZEN_2  0x50000


typedef enum PspGeneration {
  ZEN = 0,
  ZEN_PLUS,
  ZEN2,
} PspGeneration;

typedef struct AmdPspConfiguration {
    PspGeneration gen;
    uint32_t sram_size;
    uint32_t sram_base;
    uint32_t rom_size;
    uint32_t rom_base;
    uint32_t sts_base;
    uint32_t smn_ctrl_base;
    uint32_t smn_container_base;
    uint32_t smn_flash_base;
} AmdPspConfiguration;

typedef struct AmdPspState {
  /*< private >*/
  DeviceState parent_obj;
  /*< public >*/

  MemoryRegion sram;
  MemoryRegion rom;

  bool dbg_mode;
  ARMCPU cpu;

  /* This device covers every MMIO address we have not covered somewhere else */
  PSPMiscState base_mem;

  /* This device represents the SMN address space including control registers */
  PSPSmnState smn;

  /* This device represents the x86 address space */
  //PSPX86State x86;

  /* PSP Timer 1 at 0x03010400*/
  PSPTimerState timer1;

  /* PSP Timer 2 at 0x03010424*/
  PSPTimerState timer2;

  /* PSP Status port */
  PSPStsState sts;

  /* CCP */
  CcpV5State ccp;

  /* Fuse */
  PSPFuseState fuse;

  /* General unimplemented devices */
  UnimplementedDeviceState unimp;

} AmdPspState;

const char* PspGenToName(PspGeneration gen);
PspGeneration PspNameToGen(const char* name);

#endif
