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
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/module.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "hw/sysbus.h"
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/qdev-properties.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "hw/arm/psp.h"
#include "qemu/log.h"
#include "trace-hw_arm.h"
#include "trace.h"
#include "hw/arm/psp-sts.h"

static uint64_t psp_sts_read(void *opaque, hwaddr offset, unsigned int size) {
    PSPStsState *s = PSP_STS(opaque);
    trace_psp_sts_read(s->psp_sts_val);
    return s->psp_sts_val;
}

static void psp_sts_write(void *opaque, hwaddr offset,
                       uint64_t value, unsigned int size) {
    PSPStsState *s = PSP_STS(opaque);
    s->psp_sts_val = value;
    trace_psp_sts_write(value);
}

static const MemoryRegionOps sts_mem_ops = {
    .read = psp_sts_read,
    .write = psp_sts_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
};

static void psp_sts_init(Object *obj) {
    PSPStsState *s = PSP_STS(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    s->psp_sts_val = 0;

    memory_region_init_io(&s->iomem, obj, &sts_mem_ops, s,
            TYPE_PSP_STS, 4);

    sysbus_init_mmio(sbd, &s->iomem);

}

static const TypeInfo psp_sts_info = {
    .name = TYPE_PSP_STS,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = psp_sts_init,
    .instance_size = sizeof(PSPStsState),

};

static void psp_sts_register_types(void) {
    type_register_static(&psp_sts_info);

}
type_init(psp_sts_register_types);
