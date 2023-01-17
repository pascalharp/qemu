/*
 * AMD PSP Fuse emulation
 *
 * Copyright (C) 2022 Pascal Harprecht <p.harprecht@gmail.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef PSP_FUSE_H
#define PSP_FUSE_H

#include "hw/sysbus.h"
#include "exec/memory.h"

#define TYPE_PSP_FUSE "amd_psp.fuse"
OBJECT_DECLARE_SIMPLE_TYPE(PSPFuseState, PSP_FUSE)

#define PSP_FUSE_SIZE 0x4

struct PSPFuseState {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    MemoryRegion iomem;

    bool dbg_mode;
};

#endif
