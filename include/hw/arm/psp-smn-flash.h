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
#ifndef AMD_PSP_SMN_FLASH_H
#define AMD_PSP_SMN_FLASH_H

#include "hw/sysbus.h"
#include "exec/memory.h"

/* TODO: Configurable size based on image */
#define PSP_SMN_FLASH_SIZE_16 16 * 1024 * 1024
#define PSP_SMN_FLASH_SIZE_32 2 * PSP_SMN_FLASH_SIZE_16

#define TYPE_PSP_SMN_FLASH "amd_psp.smnflash"
#define PSP_SMN_FLASH(obj) OBJECT_CHECK(PSPSmnFlashState, (obj), TYPE_PSP_SMN_FLASH)

typedef struct PSPSmnFlashState {
    SysBusDevice parent_obj;

    MemoryRegion psp_smn_flash;

    char * flash_img;


} PSPSmnFlashState;
#endif
