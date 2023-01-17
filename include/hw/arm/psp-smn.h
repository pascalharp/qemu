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
#ifndef AMD_PSP_SMN_H
#define AMD_PSP_SMN_H

#include "hw/sysbus.h"
#include "exec/memory.h"
//#include "hw/arm/psp-misc.h"
#include "hw/arm/psp-smn-flash.h"
#include "hw/arm/psp-smn-misc.h"

#define PSP_SMN_CTRL_SIZE 16 * 4
#define PSP_SMN_SLOT_SIZE 1024 * 1024 /* 1M */
#define PSP_SMN_SLOT_COUNT 32
#define PSP_SMN_SLOT_NAME_LEN  20
#define PSP_SMN_SLOT_NAME "smn-slot_"

#define TYPE_PSP_SMN "amd_psp.smn"
#define TYPE_PSP_SMN_ZEN "amd_psp.smn.zen"
#define TYPE_PSP_SMN_ZEN_PLUS "amd_psp.smn.zen+"
#define TYPE_PSP_SMN_ZEN2 "amd_psp.smn.zen2"
#define PSP_SMN(obj) OBJECT_CHECK(PSPSmnState, (obj), TYPE_PSP_SMN)


typedef uint32_t PSPSmnAddr;

typedef struct PSPSmnState {
    SysBusDevice parent_obj;

    /* MemoryRegion containing the MMIO control registers */
    MemoryRegion psp_smn_control;

    /* MemoryRegions containing the SMN slots */
    MemoryRegion psp_smn_containers[PSP_SMN_SLOT_COUNT];

    /* Full 32bit SMN address space */
    MemoryRegion psp_smn_space;

    /* PSP Misc device that covers the whole SMN address space */
    //PSPMiscState psp_smn_misc;
    PSPSmnMiscState psp_smn_misc;

    /* Base address of the smn range. */
    hwaddr psp_smn_base;

    /* Current SMN state */
    PSPSmnAddr psp_smn_slots[32];

    /* The SMN attached flash */
    PSPSmnFlashState psp_smn_flash;

} PSPSmnState;
#endif
