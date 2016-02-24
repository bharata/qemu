/*
 * CPU core abstract device
 *
 * Copyright (C) 2016 Bharata B Rao <bharata@linux.vnet.ibm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */
#include "hw/cpu/core.h"

static char *core_prop_get_slot(Object *obj, Error **errp)
{
    CPUCore *core = CPU_CORE(obj);

    return g_strdup(core->slot);
}

static void core_prop_set_slot(Object *obj, const char *val, Error **errp)
{
    CPUCore *core = CPU_CORE(obj);

    core->slot = g_strdup(val);
}

static void cpu_core_instance_init(Object *obj)
{
    object_property_add_str(obj, "slot", core_prop_get_slot, core_prop_set_slot,
                            NULL);
}

static const TypeInfo cpu_core_type_info = {
    .name = TYPE_CPU_CORE,
    .parent = TYPE_DEVICE,
    .abstract = true,
    .instance_size = sizeof(CPUCore),
    .instance_init = cpu_core_instance_init,
};

static void cpu_core_register_types(void)
{
    type_register_static(&cpu_core_type_info);
}

type_init(cpu_core_register_types)
