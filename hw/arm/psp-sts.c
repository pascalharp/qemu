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
#include "qemu/log.h"
#include "hw/arm/psp-sts.h"
#include "trace.h"

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

static void psp_sts_reset(DeviceState *dev) {

    PSPStsState *s = PSP_STS(dev);
    s->psp_sts_val = 0;
}

static void psp_sts_realize(DeviceState *dev, Error **errp) {

    PSPStsState *s = PSP_STS(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    s->psp_sts_val = 0;

    memory_region_init_io(&s->iomem, OBJECT(dev), &sts_mem_ops, s,
            TYPE_PSP_STS, 4);

    sysbus_init_mmio(sbd, &s->iomem);
}

static void psp_sts_class_init(ObjectClass * klass, void * data) {
    DeviceClass * dc = DEVICE_CLASS(klass);

    dc->realize = psp_sts_realize;
    dc->reset = psp_sts_reset;
}

static const TypeInfo psp_sts_info = {
    .name = TYPE_PSP_STS,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(PSPStsState),
    .class_init = psp_sts_class_init,
};

static void psp_sts_register_types(void) {
    type_register_static(&psp_sts_info);

}
type_init(psp_sts_register_types);
