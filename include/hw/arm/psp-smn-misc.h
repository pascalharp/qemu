/* Copyright (C) 2022 Pascal Harprecht <p.harprecht@gmail.com>
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
#ifndef AMD_PSP_SMN_MISC_H
#define AMD_PSP_SMN_MISC_H

#include "hw/sysbus.h"

#define TYPE_PSP_SMN_MISC "amd_psp.smn.misc"
#define PSP_SMN_MISC(obj) OBJECT_CHECK(PSPSmnMiscState, (obj), TYPE_PSP_SMN_MISC)

typedef struct PSPSmnMiscState {
    /* <private> */
    SysBusDevice parent;
    /* <public> */
    MemoryRegion iomem;
} PSPSmnMiscState;

#endif
