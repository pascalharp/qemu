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
#include "qom/object.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "hw/sysbus.h"
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/qdev-properties.h"
#include "hw/boards.h"
#include "hw/char/serial.h"
#include "hw/loader.h"
#include "hw/arm/psp.h"
#include "qemu/log.h"
#include "hw/arm/psp-x86.h"

/* TODO: Refactor */
//static PSPMiscReg psp_regs[] = {
//    {
//        /* The off chip bootloader waits for bits 0-2 to be set. */
//        .addr = 0xfed81e77,
//        .val = 0x7,
//    },
//
//};
//
/* TODO: Refactor read/write methods */
/* TODO: fix log -> use log trace for non errors */

static uint32_t psp_x86_ctrl_read(PSPX86State *s, uint32_t slot_id,
                                  PSPX86RegId reg_id) {
  PSPX86Slot *slot;
  uint32_t ret;

  ret = 0;

  if (slot_id >= PSP_X86_SLOT_COUNT) {
        qemu_log_mask(LOG_GUEST_ERROR, "PSP X86 Error: Unknown X86 slot: %d\n",
                  slot_id);
  } else {
        slot = &s->psp_x86_slots[slot_id];
        ret = slot->ctrl_regs[reg_id];
        qemu_log_mask(LOG_UNIMP, "PSP X86: Read slot %d reg %d val 0x%x\n",
                      slot_id, reg_id, ret);
  }

  return ret;
}

static void psp_x86_map_slot(PSPX86State *s, uint32_t slot_id) {
    PSPX86Slot *slot;

    slot = &s->psp_x86_slots[slot_id];

    slot->x86_addr = (slot->ctrl_regs[X86BASE] & 0x3f) << 26 | ((uint64_t)(slot->ctrl_regs[X86BASE] >> 6)) << 32;

    memory_region_set_alias_offset(&s->psp_x86_containers[slot_id], slot->x86_addr);




}

static void psp_x86_ctrl_write(PSPX86State *s, uint32_t slot_id, PSPX86RegId reg_id,
                               uint32_t val) {
    PSPX86Slot *slot;

    if (slot_id >= PSP_X86_SLOT_COUNT) {
        qemu_log_mask(LOG_GUEST_ERROR, "PSP X86 Error: Unknown X86 slot: %d\n",
                  slot_id);
    } else {
        slot = &s->psp_x86_slots[slot_id];
        slot->ctrl_regs[reg_id] = val;

        if(reg_id == X86BASE) {
            psp_x86_map_slot(s, slot_id);

        }
        qemu_log_mask(LOG_UNIMP, "PSP X86: Write slot %d reg %d val 0x%x\n",
                      slot_id, reg_id, val);

    }
}

static void psp_x86_ctrl1_write(void *opaque, hwaddr offset, uint64_t value,
                          unsigned int size) {
    uint32_t slot_id;
    uint32_t reg_id;
    PSPX86State *s;

    s = PSP_X86(opaque);

    slot_id = offset / ( 4 * sizeof(uint32_t));
    reg_id = offset % (4 * sizeof(uint32_t));

    if (size != sizeof(uint32_t)) {
        qemu_log_mask(LOG_GUEST_ERROR, "PSP X86 Error: Unimplemented register" \
                      " acces size %d\n", size);
    } else {
        psp_x86_ctrl_write(s, slot_id, reg_id, value);

    }
}

static uint64_t psp_x86_ctrl1_read(void *opaque, hwaddr offset, unsigned int size) {
    uint32_t slot_id;
    uint32_t reg_id;
    uint32_t ret;
    PSPX86State *s;

    ret = 0;
    s = PSP_X86(opaque);

    slot_id = offset / ( 4 * sizeof(uint32_t));
    reg_id = offset % (4 * sizeof(uint32_t));

    if (size != sizeof(uint32_t)) {
        qemu_log_mask(LOG_GUEST_ERROR, "PSP X86 Error: Unimplemented register" \
                      " acces size %d\n", size);
    } else {
        ret = psp_x86_ctrl_read(s, slot_id, reg_id);

    }
    return ret;
}

static void psp_x86_ctrl2_write(void *opaque, hwaddr offset, uint64_t value,
                          unsigned int size) {
    uint32_t slot_id;
    uint32_t reg_id;
    PSPX86State *s;

    s = PSP_X86(opaque);

    slot_id = offset / sizeof(uint32_t);
    reg_id = X86_UNKNOWN4;

    if (size != sizeof(uint32_t)) {
        qemu_log_mask(LOG_GUEST_ERROR, "PSP X86 Error: Unimplemented register" \
                      " access size %d\n", size);
    } else {
        psp_x86_ctrl_write(s, slot_id, reg_id, value);
    }

}

static uint64_t psp_x86_ctrl2_read(void *opaque, hwaddr offset, unsigned int size) {
    uint32_t slot_id;
    uint32_t reg_id;
    uint32_t ret;
    PSPX86State *s;

    s = PSP_X86(opaque);

    slot_id = offset / sizeof(uint32_t);
    reg_id = X86_UNKNOWN4;

    if (size != sizeof(uint32_t)) {
        qemu_log_mask(LOG_GUEST_ERROR, "PSP X86 Error: Unimplemented register" \
                      " acces size %d\n", size);
        ret = 0;
    } else {
        ret = psp_x86_ctrl_read(s, slot_id, reg_id);
    }

    return ret;
}

static void psp_x86_ctrl3_write(void *opaque, hwaddr offset, uint64_t value,
                          unsigned int size) {
    uint32_t slot_id;
    uint32_t reg_id;
    PSPX86State *s;

    s = PSP_X86(opaque);

    slot_id = offset / sizeof(uint32_t);
    reg_id = X86_UNKNOWN5;

    if (size != sizeof(uint32_t)) {
        qemu_log_mask(LOG_GUEST_ERROR, "PSP X86 Error: Unimplemented register" \
                      " access size %d\n", size);
    } else {
        psp_x86_ctrl_write(s, slot_id, reg_id, value);
    }

}

static uint64_t psp_x86_ctrl3_read(void *opaque, hwaddr offset, unsigned int size) {

    uint32_t slot_id;
    uint32_t reg_id;
    uint32_t ret;
    PSPX86State *s;

    s = PSP_X86(opaque);

    slot_id = offset / sizeof(uint32_t);
    reg_id = X86_UNKNOWN5;

    if (size != sizeof(uint32_t)) {
        qemu_log_mask(LOG_GUEST_ERROR, "PSP X86 Error: Unimplemented register" \
                      " acces size %d\n", size);
        ret = 0;
    } else {
        ret = psp_x86_ctrl_read(s, slot_id, reg_id);
    }

    return ret;
}


static const MemoryRegionOps x86_ctlr1_ops = {
    .read = psp_x86_ctrl1_read,
    .write = psp_x86_ctrl1_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const MemoryRegionOps x86_ctlr2_ops = {
    .read = psp_x86_ctrl2_read,
    .write = psp_x86_ctrl2_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static const MemoryRegionOps x86_ctlr3_ops = {
    .read = psp_x86_ctrl3_read,
    .write = psp_x86_ctrl3_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static void psp_x86_init(Object *obj) {

    PSPX86State *s = PSP_X86(obj);

    /* Base address of the X86 containers in the PSP address space */
    object_property_add_uint64_ptr(obj,"x86-container-base", &s->psp_x86_base,
                                   OBJ_PROP_FLAG_READWRITE);

    object_initialize_child(obj, "x86_misc", &s->psp_x86_misc,
                            TYPE_PSP_MISC);

    object_property_set_uint(OBJECT(&s->psp_x86_misc), "psp_misc_msize",
                             (1UL << 48), &error_abort);

    object_property_set_str(OBJECT(&s->psp_x86_misc),"psp_misc_ident",
                            "X86 MEM", &error_abort);

    object_property_set_bool(OBJECT(&s->psp_x86_misc), "realized", true,
                             &error_abort);
}

static void psp_x86_init_slots(DeviceState *dev) {
    PSPX86State *s = PSP_X86(dev);

    int i;
    char name[PSP_X86_SLOT_NAME_LEN] = { 0 };
    hwaddr slot_offset;

    for(i = 0; i < PSP_X86_SLOT_COUNT; i++) {
        /* The X86 MMIO region of the PSP */
        snprintf(name, PSP_X86_SLOT_NAME_LEN, "%s%d", PSP_X86_SLOT_NAME, i);

        /* TODO: How to check for errors? */
        /* Each container is an alias to the X86 addres space. Here we
         * initialize each container beginning with the address 0 in the x86
         * address space.
         */
        memory_region_init_alias(&s->psp_x86_containers[i], OBJECT(dev), name,
                                 &s->psp_x86_space, i * PSP_X86_SLOT_SIZE,
                                 PSP_X86_SLOT_SIZE);

        /* Map the containers to the PSP address space */
        slot_offset = s->psp_x86_base + i * PSP_X86_SLOT_SIZE;

        memory_region_add_subregion_overlap(get_system_memory(), slot_offset,
                                            &s->psp_x86_containers[i], 0);

    }
}

static void psp_x86_realize(DeviceState *dev, Error **errp) {
    PSPX86State *s = PSP_X86(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    MemoryRegion *mr_x86_misc;

    /* The X86 control register regions */
    memory_region_init_io(&s->psp_x86_control1, OBJECT(dev), &x86_ctlr1_ops,
                          s, "psp-x86-ctrl1", PSP_X86_CTLR1_SIZE);

    sysbus_init_mmio(sbd, &s->psp_x86_control1);

    memory_region_init_io(&s->psp_x86_control2, OBJECT(dev), &x86_ctlr2_ops,
                          s, "psp-x86-ctrl2", PSP_X86_CTLR2_SIZE);

    sysbus_init_mmio(sbd, &s->psp_x86_control2);

    memory_region_init_io(&s->psp_x86_control3, OBJECT(dev), &x86_ctlr3_ops,
                          s, "psp-x86-ctrl3", PSP_X86_CTLR3_SIZE);

    sysbus_init_mmio(sbd, &s->psp_x86_control3);

    /* The X86 address space. Independent from the PSP address space */
    memory_region_init(&s->psp_x86_space, OBJECT(dev), "x86-address-space",
                       (1UL << 48) /* 48 physical address bits */);

    /* A MMIO accessible UART */
    serial_mm_init(&s->psp_x86_space, 0xfffdfc0003f8, 0,
                   0, 115200, serial_hd(0), DEVICE_NATIVE_ENDIAN);

    /* Connect the misc device to the x86 address space */
    /* TODO: FIX */
    //s->psp_x86_misc.regs = psp_regs;
    //s->psp_x86_misc.regs_count = ARRAY_SIZE(psp_regs);

    mr_x86_misc = sysbus_mmio_get_region(SYS_BUS_DEVICE(&s->psp_x86_misc), 0);
    memory_region_add_subregion_overlap(&s->psp_x86_space, 0x0, mr_x86_misc,
                                        -1000);

    psp_x86_init_slots(dev);

}

static void psp_x86_class_init(ObjectClass *oc, void *data) {
    DeviceClass *dc = DEVICE_CLASS(oc);
    dc->realize = psp_x86_realize;
}

static const TypeInfo psp_x86_info = {
    .name = TYPE_PSP_X86,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = psp_x86_init,
    .instance_size = sizeof(PSPX86State),
    .class_init = psp_x86_class_init,

};

static void psp_x86_register_types(void) {
    type_register_static(&psp_x86_info);

}
type_init(psp_x86_register_types);
