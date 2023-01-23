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
#include "hw/arm/psp-timer.h"
#include "qom/object.h"
#include "trace.h"

static char ident[] = "PSP Timer";

static uint64_t psp_timer_read(void *opaque, hwaddr offset, unsigned int size) {
    PSPTimerState *s = PSP_TIMER(opaque);
    uint64_t val;
    hwaddr phys_base = s->psp_timer_iomem.addr;

    if (size != sizeof(uint32_t)) {
        qemu_log_mask(LOG_UNIMP,
                      "%s: Error. Unsupported read size at offset 0x%" \
                      HWADDR_PRIx "\n", ident, phys_base + offset);
        return 0;
    }

    // TODO do this properly instead of counting up
    switch (offset) {
        case 0:
            val = s->psp_timer_control;
            trace_psp_timer_control_read(offset, val);
            break;
        case 0x20:
            val = s->psp_timer_count;
            trace_psp_timer_counter_read(val);
            if (s->psp_timer_control & 0x1)
                s->psp_timer_count++;
            break;
        default:
            val = 0;
            break;
    }

    return val;
}

static void psp_timer_write(void *opaque, hwaddr offset, uint64_t val,
                            unsigned int size) {
    PSPTimerState *s = PSP_TIMER(opaque);
    hwaddr phys = s->psp_timer_iomem.addr;
    phys += offset;


    if (size != sizeof(uint32_t) && size != sizeof(uint8_t)) {
        qemu_log_mask(LOG_UNIMP,
                      "PSP Timer: Error. Unsupported write size (%d) at offset 0x%" \
                      HWADDR_PRIx " with val 0x%lx\n", size, phys, val);
        return;
    }
    switch (offset) {
        case 0x0:
            s->psp_timer_control = val;
            trace_psp_timer_control_write(offset, val);
            break;
        case 0x1:
            /* XXX For single byte access by the on chip bl... */
            s->psp_timer_control |= val << 8;
            trace_psp_timer_control_write(offset, val);
            break;
        case 0x20:
            s->psp_timer_count = val;
            trace_psp_timer_counter_write(val);
            break;
        default:
            qemu_log_mask(LOG_UNIMP, "PSP Timer. Error: Unsupported MMIO accces." \
                          "offset=0x%" HWADDR_PRIx " value 0x%lx\n", offset,
                          val);
            break;

    }
}


static const MemoryRegionOps timer_mem_ops = {
    .read = psp_timer_read,
    .write = psp_timer_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static void psp_timer_realize(DeviceState *dev, Error **errp) {
    PSPTimerState *s = PSP_TIMER(dev);

    s->psp_timer_control = 0;
    s->psp_timer_count = 0;

    memory_region_init_io(&s->psp_timer_iomem, OBJECT(dev), &timer_mem_ops, s,
                          TYPE_PSP_TIMER, PSP_TIMER_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->psp_timer_iomem);
}

static void psp_timer_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = psp_timer_realize;
}

static const TypeInfo psp_timer_info = {
    .name = TYPE_PSP_TIMER,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(PSPTimerState),
    .class_init = psp_timer_class_init,
};

static void psp_timer_register_types(void) {
    type_register_static(&psp_timer_info);

}
type_init(psp_timer_register_types);
