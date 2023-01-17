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
#include "qemu/error-report.h"
#include "qemu/module.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "hw/sysbus.h"
#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qapi/error.h"
#include "hw/qdev-properties.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "hw/arm/psp.h"

/* Setup inspired from raspi.c */

typedef struct AmdPspMachineState {
    /*< private >*/
    MachineState parent_obj;
    /*< public >*/
    AmdPspState soc;
} AmdPspMachineState;

typedef struct AmdPspMachineClass {
    /*< private >*/
    MachineClass parent_obj;
    /*< public >*/
    PspGeneration gen;
} AmdPspMachineClass;

static const char * soc_versions[] = {
    [ZEN]      = TYPE_AMD_PSP_ZEN,
    [ZEN_PLUS] = TYPE_AMD_PSP_ZEN_PLUS,
    [ZEN2]     = TYPE_AMD_PSP_ZEN_TWO
};

#define TYPE_AMD_PSP_MACHINE MACHINE_TYPE_NAME("amd-psp-common")
DECLARE_OBJ_CHECKERS(AmdPspMachineState, AmdPspMachineClass,
                     AMD_PSP_MACHINE, TYPE_AMD_PSP_MACHINE)

static void zen_init_common(MachineState *machine) {
    AmdPspMachineState *ms = AMD_PSP_MACHINE(machine);
    AmdPspMachineClass *mc = AMD_PSP_MACHINE_GET_CLASS(machine);

    /* Initialize Soc */
    object_initialize_child(OBJECT(machine), "soc", &ms->soc, soc_versions[mc->gen]);
    qdev_realize(DEVICE(&ms->soc), NULL, &error_fatal);

    /* Set pc, Is there a more elegent way?
     * TODO Consider using arm_boot_info from boot.h
     */
    ms->soc.cpu.env.regs[15] = 0xffff0000;

    /* TODO trigger on -kernel flag if it is provided */
    /* TODO can we maybe use -pflash flag for off chip bl? */
    /* TODO consider emulating/skipping on-chip-bl if not set */
}

static void psp_zen_machine_common_init(MachineClass *mc) {
    mc->desc = "AMD PSP";
    mc->init = zen_init_common;
    mc->block_default_type = IF_NONE;
    mc->min_cpus = mc->max_cpus = mc->default_cpus = 1;
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-a9");
    /* 
     * hw/core/generic_loader.c     line 157
     * The PSP does not have ram, however the generic-loader device apparently
     * validates this value, so we set it here to an sufficiently large value
     * TODO: Verify that this has not other, unwanted side-effects.
     */
    mc->default_ram_size = 1 * GiB;
    mc->default_ram_id = "psp-ram";
}

static void psp_zen_machine_init(ObjectClass *oc, void *data) {
    MachineClass *mc = MACHINE_CLASS(oc);
    AmdPspMachineClass *apmc = AMD_PSP_MACHINE_CLASS(oc);
    apmc->gen = ZEN;
    psp_zen_machine_common_init(mc);
}

static void psp_zen_plus_machine_init(ObjectClass *oc, void *data) {
    MachineClass *mc = MACHINE_CLASS(oc);
    AmdPspMachineClass *apmc = AMD_PSP_MACHINE_CLASS(oc);
    apmc->gen = ZEN_PLUS;
    psp_zen_machine_common_init(mc);
}

static void psp_zen_two_machine_init(ObjectClass *oc, void *data) {
    MachineClass *mc = MACHINE_CLASS(oc);
    AmdPspMachineClass *apmc = AMD_PSP_MACHINE_CLASS(oc);
    apmc->gen = ZEN2;
    psp_zen_machine_common_init(mc);
}

static const TypeInfo amd_psp_machine_types[] = {
    {
        .name           = MACHINE_TYPE_NAME("amd-psp-zen"),
        .parent         = TYPE_AMD_PSP_MACHINE,
        .class_init     = psp_zen_machine_init,
    }, {
        .name           = MACHINE_TYPE_NAME("amd-psp-zen+"),
        .parent         = TYPE_AMD_PSP_MACHINE,
        .class_init     = psp_zen_plus_machine_init,
    }, {
        .name           = MACHINE_TYPE_NAME("amd-psp-zen2"),
        .parent         = TYPE_AMD_PSP_MACHINE,
        .class_init     = psp_zen_two_machine_init,
    }, {
        .name           = TYPE_AMD_PSP_MACHINE,
        .parent         = TYPE_MACHINE,
        .instance_size  = sizeof(AmdPspMachineState),
        .class_size     = sizeof(AmdPspMachineClass),
        .abstract       = true,
    }
};

DEFINE_TYPES(amd_psp_machine_types);
