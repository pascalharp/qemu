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
#include "hw/arm/psp-misc.h"
#include "hw/arm/psp-smn.h"
#include "hw/arm/psp-x86.h"
#include "hw/arm/psp-timer.h"
#include "hw/arm/psp-sts.h"
#include "qemu/log.h"
#include <stdint.h>

// TODO: use mmio_map_overlap with memory_regions_dispatch_rw to log access to SPI flash

static const char * amd_psp_gen_ident[] = {
    [ZEN] = "Ryzen Zen",
    [ZEN_PLUS] = "Ryzen Zen Plus",
    [ZEN2] = "Ryzen Zen 2"
};

/* The base class */
typedef struct AmdPspClass {
    /*< private >*/
    DeviceClass parent_class;
    /*< public >*/
    uint64_t sram_size;
    hwaddr sram_base;
    uint64_t rom_size;
    hwaddr rom_base;

    hwaddr sts_base;

    const char * smn_type;
    hwaddr smn_ctrl_base;
    hwaddr smn_container_base;
} AmdPspClass;

#define AMD_PSP_CLASS(klass) \
    OBJECT_CLASS_CHECK(AmdPspClass, (klass), TYPE_AMD_PSP)
#define AMD_PSP_GET_CLASS(obj) \
    OBJECT_GET_CLASS(AmdPspClass, (obj), TYPE_AMD_PSP)

// TODO: Check CPU Object properties

/* Initialize psp and its device. This should never fail */
static void amd_psp_init(Object *obj)
{

    AmdPspState *s = AMD_PSP(obj);
    AmdPspClass *c = AMD_PSP_GET_CLASS(obj);

    object_initialize_child(obj, "cpu", &s->cpu,
                            ARM_CPU_TYPE_NAME("cortex-a9"));

    /*
     * Based on the reset in target/arm/cpu_tcg.c cortex_a9_initfn
     * Deactivates all flags except V bit (Hivecs)
     */
    s->cpu.reset_sctlr = 0x00c52078;

    object_initialize_child(obj, "smn", &s->smn, c->smn_type);

    //object_initialize_child(obj, "x86", &s->x86, TYPE_PSP_X86);

    object_initialize_child(obj, "base_mem", &s->base_mem, TYPE_PSP_MISC);

    object_initialize_child(obj, "timer1", &s->timer1,TYPE_PSP_TIMER);

    object_initialize_child(obj, "timer2", &s->timer2, TYPE_PSP_TIMER);

    object_initialize_child(obj, "psp-sts", &s->sts, TYPE_PSP_STS);

    object_initialize_child(obj, "psp-ccp", &s->ccp, TYPE_CCP_V5);

    object_initialize_child(obj, "psp-fuse", &s->fuse, TYPE_PSP_FUSE);

    object_initialize_child(obj, "unimp", &s->unimp, TYPE_UNIMPLEMENTED_DEVICE);
}

/* Realize the psp and configure devices */
static void amd_psp_realize(DeviceState *dev, Error **errp)
{
    AmdPspState *s = AMD_PSP(dev);
    AmdPspClass *c = AMD_PSP_GET_CLASS(dev);

    qemu_log("Generation: %s\n", DEVICE_GET_CLASS(dev)->desc);

    /* TODO: Maybe use "&error_abort" instead? */
    Error *err = NULL;

    /* TODO: Enable the ARM TrustZone extensions. Needed? */
    //object_property_set_bool(OBJECT(&s->cpu), "has_el3", true,  &err);

    /* Init CPU object. TODO convert to qdev_init_nofail */
    qdev_realize(DEVICE(&s->cpu), NULL, &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }

    //TODO: Do we really need to solve this with a prop?
    qdev_prop_set_int32(DEVICE(&s->smn), "smn-container-base", c->smn_container_base);
    if(!sysbus_realize(SYS_BUS_DEVICE(&s->smn), errp)) {
        return;
    }

    /* TODO: Register x86 address-spaces */
    //object_property_set_uint(OBJECT(&s->x86), "x86-container-base",
    //                        PSP_X86_BASE, &error_abort);
    //sysbus_realize(SYS_BUS_DEVICE(&s->x86), &err);

    if(!sysbus_realize(SYS_BUS_DEVICE(&s->base_mem), &err)) {
        return;
    }

    /* Init SRAM */
    memory_region_init_ram(&s->sram, OBJECT(dev), "sram", c->sram_size,
                           &error_abort);
    memory_region_add_subregion(get_system_memory(), c->sram_base, &s->sram);

    /* Init ROM */
    memory_region_init_rom(&s->rom, OBJECT(dev), "rom", c->rom_size,
                           &error_abort);
    memory_region_add_subregion(get_system_memory(), c->rom_base, &s->rom);

    /* Map SMN control registers */
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->smn), 0, c->smn_ctrl_base);

    /* Map X86 control registers */
    //sysbus_mmio_map(SYS_BUS_DEVICE(&s->x86), 0, PSP_X86_CTRL1_BASE);

    /* Map X86 control registers */
    //sysbus_mmio_map(SYS_BUS_DEVICE(&s->x86), 1, PSP_X86_CTRL2_BASE);

    /* Map X86 control registers */
    //sysbus_mmio_map(SYS_BUS_DEVICE(&s->x86), 2, PSP_X86_CTRL3_BASE);

    /* Map timer 1 */
    if(!sysbus_realize(SYS_BUS_DEVICE(&s->timer1), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->timer1), 0, PSP_TIMER1_BASE);

    /* Map timer 2 */
    if(!sysbus_realize(SYS_BUS_DEVICE(&s->timer2), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->timer2), 0, PSP_TIMER2_BASE);

    /* Map PSP Status port */
    if(!sysbus_realize(SYS_BUS_DEVICE(&s->sts), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->sts), 0, c->sts_base);

    /* Map CCP */
    if(!sysbus_realize(SYS_BUS_DEVICE(&s->ccp), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->ccp), 0, PSP_CCP_BASE);

    /* Map Fuse */
    if(s->dbg_mode) {
        /* set dbg_mode if global dbg mode is active */
        qdev_prop_set_bit(DEVICE(&s->fuse), "dbg_mode", true);
    }
    if(!sysbus_realize(SYS_BUS_DEVICE(&s->fuse), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->fuse), 0, PSP_FUSE_BASE);

    /* Map the misc device as an overlap with low priority */
    /* This device covers all "unknown" psp registers */
    sysbus_mmio_map_overlap(SYS_BUS_DEVICE(&s->base_mem), 0, PSP_MMIO_BASE, -100);

    /* General unimplemented device that maps the whole address-spaces with low priority */
    qdev_prop_set_string(DEVICE(&s->unimp), "name", "unimp");
    qdev_prop_set_uint64(DEVICE(&s->unimp), "size", UINT32_MAX);
    if(!sysbus_realize(SYS_BUS_DEVICE(&s->unimp), &error_abort)) {
        return;
    }
    sysbus_mmio_map_overlap(SYS_BUS_DEVICE(&s->unimp), 0, 0, -1000);

}

/* User-configurable options with "-global amd-psp.<property>=<value> */
static Property amd_psp_properties[] = {
    DEFINE_PROP_BOOL("dbg_mode", AmdPspState, dbg_mode, false),
    DEFINE_PROP_END_OF_LIST(),
};

static void amd_psp_class_init(ObjectClass *oc, void* data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    device_class_set_props(dc,amd_psp_properties);
    dc->realize = amd_psp_realize;
    /* Disabling device instantiation with arbitrary machines */
    dc->user_creatable = false;
}

static void amd_psp_zen_class_init(ObjectClass *oc, void *data) {
    DeviceClass *dc = DEVICE_CLASS(oc);
    AmdPspClass *pspc = AMD_PSP_CLASS(oc);

    dc->desc = amd_psp_gen_ident[ZEN];

    pspc->sram_size = (256 * 1024);
    pspc->sram_base = 0x0;

    pspc->rom_size = (64 * 1024);
    pspc->rom_base = 0xffff0000;

    pspc->smn_type = TYPE_PSP_SMN_ZEN;
    pspc->smn_ctrl_base = 0x03220000;
    pspc->smn_container_base = 0x01000000;


    pspc->sts_base = 0x032000e8;
}

static void amd_psp_zen_plus_class_init(ObjectClass *oc, void *data) {
    DeviceClass *dc = DEVICE_CLASS(oc);
    /* Zen Plus barely differs from orig Zen. Overwrite differences afterwards */
    amd_psp_zen_class_init(oc, data);
    dc->desc = amd_psp_gen_ident[ZEN_PLUS];
}

static void amd_psp_zen_two_class_init(ObjectClass *oc, void *data) {
    DeviceClass *dc = DEVICE_CLASS(oc);
    AmdPspClass *pspc = AMD_PSP_CLASS(oc);

    dc->desc = amd_psp_gen_ident[ZEN2];

    // TODO set/verify values for Zen 2
    pspc->sram_size = (320 * 1024);
    pspc->sram_base = 0x0;

    pspc->rom_size = (64 * 1024);
    pspc->rom_base = 0xffff0000;

    pspc->smn_type = TYPE_PSP_SMN_ZEN2;
    pspc->smn_ctrl_base = 0x03220000;
    pspc->smn_container_base = 0x01000000;

    pspc->sts_base = 0x032000d8;
}

static const TypeInfo amd_psp_types[] = {
    {
        .name           = TYPE_AMD_PSP_ZEN,
        .parent         = TYPE_AMD_PSP,
        .class_init     = amd_psp_zen_class_init,
    }, {
        .name           = TYPE_AMD_PSP_ZEN_PLUS,
        .parent         = TYPE_AMD_PSP,
        .class_init     = amd_psp_zen_plus_class_init,
    }, {
        .name           = TYPE_AMD_PSP_ZEN_TWO,
        .parent         = TYPE_AMD_PSP,
        .class_init     = amd_psp_zen_two_class_init,
    }, {
        .name           = TYPE_AMD_PSP,
        .parent         = TYPE_DEVICE,
        .instance_size  = sizeof(AmdPspState),
        .instance_init  = amd_psp_init,
        .class_size     = sizeof(AmdPspClass),
        .class_init     = amd_psp_class_init,
        .abstract       = true,
    }
};

DEFINE_TYPES(amd_psp_types)
