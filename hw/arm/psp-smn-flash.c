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
#include "hw/arm/psp-smn-flash.h"

/* static const MemoryRegionOps smn_flash_ops = { */
/*     .read = psp_smn_flash_read, */
/*     .write = psp_smn_flash_write, */
/*     .endianness = DEVICE_LITTLE_ENDIAN, */
/*     .impl.min_access_size = 1, */
/*     .impl.max_access_size = 4, */
/* }; */

static void psp_smn_flash_realize(DeviceState *dev, Error **errp)
{
    PSPSmnFlashState *s = PSP_SMN_FLASH(dev);
    Object *obj = OBJECT(dev);

    memory_region_init_ram(&s->psp_smn_flash, obj, "flash",
                           PSP_SMN_FLASH_SIZE_16, errp);

    if(!s->flash_img) {
        qemu_log_mask(LOG_GUEST_ERROR, "No flash image provided for smn\n");
        exit(1);
    }

    if(load_image_mr(s->flash_img, &s->psp_smn_flash) < 0 ) {
        qemu_log_mask(LOG_GUEST_ERROR, "Could not load flash image provided for smn\n");
        exit(1);
    }
}

static Property psp_smn_flash_properties[] = {
    DEFINE_PROP_STRING("flash_img", PSPSmnFlashState, flash_img),
    DEFINE_PROP_END_OF_LIST(),
};

static void psp_smn_flash_class_init(ObjectClass *klass, void * data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = psp_smn_flash_realize;
    device_class_set_props(dc, psp_smn_flash_properties);
}

static const TypeInfo psp_smn_flash_info = {
    .name = TYPE_PSP_SMN_FLASH,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(PSPSmnFlashState),
    .class_init = psp_smn_flash_class_init,
};

static void psp_smn_register_types(void) {
    type_register_static(&psp_smn_flash_info);

}
type_init(psp_smn_register_types);
