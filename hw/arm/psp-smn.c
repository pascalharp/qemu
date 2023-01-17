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
#include "qemu/log-for-trace.h"
#include "trace-hw_arm.h"
#include "trace.h"
#include "hw/arm/psp-smn.h"
#include "hw/arm/psp.h"

/* DEFINE_PROP_STRING("rom_path", AmdPspState,flash_rom_path), */

/* PSPSmnCTRL: Controller, attaches SMN devices on demand? */

typedef struct PspSmnClass {
    /*< private >*/
    SysBusDeviceClass parent_class;
    /*< public >*/
    hwaddr flash;
} PspSmnClass;

#define PSP_SMN_CLASS(klass) \
    OBJECT_CLASS_CHECK(PspSmnClass, (klass), TYPE_PSP_SMN)
#define PSP_SMN_GET_CLASS(obj) \
    OBJECT_GET_CLASS(PspSmnClass, (obj), TYPE_PSP_SMN)

typedef struct PspSmnConfiguration {
    const char * desc;
    hwaddr flash;
} PspSmnConfiguration;

static PspSmnConfiguration psp_smn_configuration[] = {
    [ZEN] = {
        .desc = "psp smn zen",
        .flash = 0x0a000000,
    },
    [ZEN_PLUS] = {
        .desc = "psp smn zen+",
        .flash = 0x0a000000,
    },
    [ZEN2] = {
        .desc = "psp smn zen2",
        .flash = 0x44000000,
    },
};

const char* ident = "SMN Control";

static void psp_smn_update_slot(PSPSmnState *smn, uint32_t idx) {
    PSPSmnAddr addr = smn->psp_smn_slots[idx];
    /* TODO documentation */
    memory_region_set_alias_offset(&smn->psp_smn_containers[idx], addr);
    trace_psp_smn_update_slot(idx, addr);

}

static void psp_smn_write(void *opaque, hwaddr offset, uint64_t value,
                          unsigned int size) {
    PSPSmnState *smn = PSP_SMN(opaque);
    uint32_t idx;
    hwaddr phys_base = smn->psp_smn_control.addr;

    trace_psp_smn_write(offset, phys_base, value, size);

    switch (size) {
        case 4:
            idx = (offset / 4) * 2;
            smn->psp_smn_slots[idx] = ((uint32_t) value & 0xffff) << 20;
            smn->psp_smn_slots[idx + 1] = ((uint32_t) value >> 16) << 20;
            psp_smn_update_slot(smn, idx);
            psp_smn_update_slot(smn, idx + 1);
            break;
        case 2:
            idx = offset / 2;
            smn->psp_smn_slots[idx] = (uint32_t) value << 20;
            psp_smn_update_slot(smn, idx);
            break;
        default:
            qemu_log_mask(LOG_UNIMP,
                          "%s: SMN ERROR. Unsupported write access size: %d",
                          ident, size);
            break;
    }
}

static uint64_t psp_smn_read(void *opaque, hwaddr offset, unsigned int size) {

    PSPSmnState *smn = PSP_SMN(opaque);
    uint32_t idx;
    uint32_t val = 0;
    hwaddr phys_base = smn->psp_smn_control.addr;

    switch (size) {
        case 4:
            /* Each 4 byte access programs two slots.
             * SMN regions are 1M large, thus each SMN address is 1M aligned.
             */
            idx = (offset / 4) * 2;
            val = ((smn->psp_smn_slots[idx + 1] >> 20) << 16) |
                  (smn->psp_smn_slots[idx] >> 20);
            break;
        default:
            qemu_log_mask(LOG_UNIMP,
                          "%s: SMN ERROR. Unsupported read access size: %d\n",
                          ident, size);
            break;

    }
    trace_psp_smn_read(offset, phys_base, val, size);
    return val;
}

static const MemoryRegionOps smn_ctlr_ops = {
    .read = psp_smn_read,
    .write = psp_smn_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static void psp_smn_init(Object *obj)
{
    PSPSmnState *s = PSP_SMN(obj);
    /* SysBusDevice *sbd = SYS_BUS_DEVICE(obj); */


    /* Base address of the SMN containers in the PSP address space */
    object_property_add_uint64_ptr(obj,"smn-container-base", &s->psp_smn_base,
                                   OBJ_PROP_FLAG_READWRITE);

    object_initialize_child(obj, "smn_misc", &s->psp_smn_misc,
                            TYPE_PSP_SMN_MISC);

    object_initialize_child(obj, "smn_flash", &s->psp_smn_flash,
                            TYPE_PSP_SMN_FLASH);
}

static void psp_smn_init_slots(DeviceState *dev) {
    PSPSmnState *s = PSP_SMN(dev);
    int i;
    char name[PSP_SMN_SLOT_NAME_LEN] = { 0 };
    hwaddr slot_offset;

    for(i = 0; i < PSP_SMN_SLOT_COUNT; i++) {
        /* The SMN MMIO region of the PSP */
        snprintf(name, PSP_SMN_SLOT_NAME_LEN, "%s%d", PSP_SMN_SLOT_NAME, i);

        /* TODO: How to check for errors? */
        /* Each container is an alias to the SMN addres space. Here we
         * initialize each container beginning with the address 0 in the SMN
         * address space.
         */
        memory_region_init_alias(&s->psp_smn_containers[i], OBJECT(dev), name,
                                 &s->psp_smn_space, i * PSP_SMN_SLOT_SIZE,
                                 PSP_SMN_SLOT_SIZE);

        /* Map the containers to the PSP address space */
        slot_offset = s->psp_smn_base + i * PSP_SMN_SLOT_SIZE;

        memory_region_add_subregion_overlap(get_system_memory(), slot_offset,
                                            &s->psp_smn_containers[i], 0);

    }

}

static void psp_smn_realize(DeviceState *dev, Error **errp) {
    PSPSmnState *s = PSP_SMN(dev);
    PspSmnClass *c = PSP_SMN_GET_CLASS(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    MemoryRegion *mr_smn_misc;
    PSPSmnFlashState* flash;
    Error *err = NULL;

    sysbus_realize(SYS_BUS_DEVICE(&s->psp_smn_misc), &err);
    sysbus_realize(SYS_BUS_DEVICE(&s->psp_smn_flash), &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }

    /* The SMN address space. Independent from the PSP address space */
    memory_region_init(&s->psp_smn_space, OBJECT(dev), "smn-address-space",
                       0xFFFFFFFF);

    /* The SMN MMIO regions containing the control regs */
    memory_region_init_io(&s->psp_smn_control, OBJECT(dev), &smn_ctlr_ops, s,
                          TYPE_PSP_SMN, PSP_SMN_CTRL_SIZE);

    sysbus_init_mmio(sbd, &s->psp_smn_control);

    mr_smn_misc = sysbus_mmio_get_region(SYS_BUS_DEVICE(&s->psp_smn_misc), 0);
    memory_region_add_subregion_overlap(&s->psp_smn_space, 0x0, mr_smn_misc,
                                        -1000);

    /* Map the flash region into the SMN address space */
    flash = &s->psp_smn_flash;
    memory_region_add_subregion_overlap(&s->psp_smn_space, c->flash,
                                        &flash->psp_smn_flash, 0);

    /* Setup the initial SMN to PSP MemoryRegion alias. */
    psp_smn_init_slots(dev);
}

static void psp_smn_class_init(ObjectClass *oc, void *data) {
    DeviceClass *dc = DEVICE_CLASS(oc);
    dc->realize = psp_smn_realize;
}

static void psp_smn_common_class_init(ObjectClass *oc, enum PspGeneration gen) {
    DeviceClass *dc = DEVICE_CLASS(oc);
    PspSmnClass *sc = PSP_SMN_CLASS(oc);

    dc->desc = psp_smn_configuration[gen].desc;
    sc->flash = psp_smn_configuration[gen].flash;
}

static void psp_smn_zen_class_init(ObjectClass *oc, void *data) {
    psp_smn_common_class_init(oc, ZEN);
}

static void psp_smn_zenplus_class_init(ObjectClass *oc, void *data) {
    psp_smn_common_class_init(oc, ZEN_PLUS);
}

static void psp_smn_zentwo_class_init(ObjectClass *oc, void *data) {
    psp_smn_common_class_init(oc, ZEN2);
}

static const TypeInfo amd_psp_smn_types[] = {
    {
        .name          = TYPE_PSP_SMN_ZEN,
        .parent        = TYPE_PSP_SMN,
        .class_init    = psp_smn_zen_class_init,
    },
    {
        .name          = TYPE_PSP_SMN_ZEN_PLUS,
        .parent        = TYPE_PSP_SMN,
        .class_init    = psp_smn_zenplus_class_init,
    },
    {
        .name          = TYPE_PSP_SMN_ZEN2,
        .parent        = TYPE_PSP_SMN,
        .class_init    = psp_smn_zentwo_class_init,
    },
    {
        .name          = TYPE_PSP_SMN,
        .parent        = TYPE_SYS_BUS_DEVICE,
        .instance_init = psp_smn_init,
        .instance_size = sizeof(PSPSmnState),
        .class_size    = sizeof(PspSmnClass),
        .class_init    = psp_smn_class_init,
        .abstract      = true,
    }
};

DEFINE_TYPES(amd_psp_smn_types);
