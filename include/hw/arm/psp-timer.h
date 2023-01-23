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
#ifndef AMD_PSP_TIMER_H
#define AMD_PSP_TIMER_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_PSP_TIMER "amd_psp.timer"
OBJECT_DECLARE_SIMPLE_TYPE(PSPTimerState, PSP_TIMER)

/* PSP Timer iomem size */
#define PSP_TIMER_SIZE 0x24

typedef struct PSPTimerState {
    SysBusDevice parent_obj;

    /* MemoryRegion containing the MMIO registers */
    MemoryRegion psp_timer_iomem;

    uint32_t psp_timer_control;
    uint32_t psp_timer_count;


} PSPTimerState;
#endif
