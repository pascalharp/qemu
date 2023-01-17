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
#ifndef AMD_PSP_X86_H
#define AMD_PSP_X86_H

#include "hw/sysbus.h"
#include "exec/memory.h"
#include "hw/arm/psp-misc.h"

#define TYPE_PSP_X86 "amd_psp.x86"
#define PSP_X86(obj) OBJECT_CHECK(PSPX86State, (obj), TYPE_PSP_X86)
#define PSP_X86_SLOT_NAME_LEN  20
#define PSP_X86_SLOT_NAME "x86-slot_"

#define PSP_X86_SLOT_SIZE 64 * 1024 * 1024 /* 64M */
#define PSP_X86_SLOT_COUNT 62
#define PSP_X86_CONTAINER_SIZE PSP_X86_SLOT_SIZE * PSP_X86_SLOT_COUNT
#define PSP_X86_REG_COUNT 6

#define PSP_X86_CTLR1_SIZE PSP_X86_SLOT_COUNT * 4 * 4
#define PSP_X86_CTLR2_SIZE PSP_X86_SLOT_COUNT * 4
#define PSP_X86_CTLR3_SIZE PSP_X86_CTLR2_SIZE

typedef enum PSPX86RegId {
  X86BASE = 0,
  X86_UNKNOWN1,
  X86_UNKNOWN2,
  X86_UNKNOWN3,
  X86_UNKNOWN4,
  X86_UNKNOWN5,
  X86_REG_MAX = PSP_X86_REG_COUNT,
} PSPX86RegId;

typedef uint64_t PSPX86Addr;
typedef struct PSPX86Slot {

  /* Currently mapped x86 address */
  PSPX86Addr x86_addr;

  /* Current mapping destination */
  hwaddr     psp_addr;

  uint32_t ctrl_regs[PSP_X86_REG_COUNT];
} PSPX86Slot;

typedef struct PSPX86State {
    SysBusDevice parent_obj;

    /* X86 Control regs */

    /* CTRL region 1: PSP_X86_SLOT_COUNT * regsize (4) * regcount (4) = 0x3e0 */
    MemoryRegion psp_x86_control1;

    /* CTRL region 2: PSP_X86_SLOT_COUNT * regsize (4) * regcount (1) = 0xf8 */
    MemoryRegion psp_x86_control2;

    /* CTRL region 3: PSP_X86_SLOT_COUNT * regsize (4) * regcount (1) = 0xf8 */
    MemoryRegion psp_x86_control3;

    /* Base address of the x86 container range. */
    hwaddr psp_x86_base;

    MemoryRegion psp_x86_containers[PSP_X86_SLOT_COUNT];

    PSPX86Slot psp_x86_slots[PSP_X86_SLOT_COUNT];

    /* PSP Misc device that covers the whole x86 address space */
    PSPMiscState psp_x86_misc;

    /* The X86 address space */
    MemoryRegion psp_x86_space;

} PSPX86State;
#endif
