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
 *
 * Special PSP device that covers various unknown MMIO regs.
 */

#include "qemu/osdep.h"
#include "hw/arm/psp-misc.h"
#include "trace.h"

/*
 * TODO consider to make this generation dependent.
 */
static int psp_misc_load_value(hwaddr addr, uint64_t * val)
{
    switch (addr){
    // Zen and Zen+
    case 0x0020004c:
        *val = 0xbc090071; break;

    // Zen2
    case 0x00200048:
        *val = 0xbc0b0552; break;

    // Zen2
    case 0x00200050:
        *val = 0x300; break;

    // TODO this should depend on dbg flag: 
    // https://github.com/PSPReverse/PSPEmu/blob/78e4be24e882ae67867063c894fbfb2ecbe50f3f/psp-dev-mmio-unknown.c#L67
    // Zen2
    case 0x000101c0:
        *val = 0x80102; break;

    default:
        /* No value found */
        return -1;
    }

    return 0;
}

static void psp_misc_write(void *opaque, hwaddr offset, uint64_t value,
                           unsigned int size)
{
    trace_psp_misc_write_unimplemented(offset, value, size);
}

static uint64_t psp_misc_read(void *opaque, hwaddr offset, unsigned int size)
{
    uint64_t value = 0;

    if(psp_misc_load_value(offset, &value)) {
        trace_psp_misc_read_unimplemented(offset, size);
    } else {
        trace_psp_misc_read(offset, value, size);
    }

    return value;
}

static const MemoryRegionOps misc_mem_ops = {
    .read = psp_misc_read,
    .write = psp_misc_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static void psp_misc_realize(DeviceState *dev, Error **errp)
{
    PSPMiscState *s = PSP_MISC(dev);

    memory_region_init_io(&s->iomem, OBJECT(dev), &misc_mem_ops, s,
                          TYPE_PSP_MISC, 0x1000000);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);
}

static void psp_misc_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    dc->realize = psp_misc_realize;
}

static const TypeInfo psp_misc_info = {
    .name = TYPE_PSP_MISC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(PSPMiscState),
    .class_init = psp_misc_class_init,
};

static void psp_misc_register_types(void)
{
    type_register_static(&psp_misc_info);
}

type_init(psp_misc_register_types);
