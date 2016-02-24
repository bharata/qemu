/*
 * sPAPR CPU core device, acts as container of CPU thread devices.
 *
 * Copyright (C) 2016 Bharata B Rao <bharata@linux.vnet.ibm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */
#include "hw/cpu/core.h"
#include "hw/ppc/spapr_cpu_core.h"
#include "hw/ppc/spapr.h"
#include "hw/boards.h"
#include "qemu/error-report.h"
#include "qapi/visitor.h"
#include <sysemu/cpus.h>

static int spapr_cpu_core_realize_child(Object *child, void *opaque)
{
    Error **errp = opaque;
    sPAPRMachineState *spapr = SPAPR_MACHINE(qdev_get_machine());
    CPUState *cs = CPU(child);
    PowerPCCPU *cpu = POWERPC_CPU(cs);

    object_property_set_bool(child, true, "realized", errp);
    if (*errp) {
        return 1;
    }

    spapr_cpu_init(spapr, cpu, errp);
    if (*errp) {
        return 1;
    }

    return 0;
}

static void spapr_cpu_core_realize(DeviceState *dev, Error **errp)
{
    sPAPRCPUCore *core = SPAPR_CPU_CORE(OBJECT(dev));
    sPAPRMachineState *spapr = SPAPR_MACHINE(qdev_get_machine());
    char *slot;
    Error *local_err = NULL;

    if (!core->nr_threads) {
        error_setg(errp, "nr_threads property can't be 0");
        return;
    }

    if (!core->oc) {
        error_setg(errp, "cpu_model property isn't set");
        return;
    }

    /*
     * TODO: If slot isn't specified, plug this core into
     * an existing empty slot.
     */
    slot = object_property_get_str(OBJECT(dev), CPU_CORE_SLOT_PROP, &local_err);
    if (!slot) {
        error_setg(errp, "slot property isn't set");
        return;
    }

    object_property_set_link(OBJECT(spapr), OBJECT(core), slot, &local_err);
    g_free(slot);
    if (local_err) {
        error_propagate(errp, local_err);
        return;
    }

    object_child_foreach(OBJECT(dev), spapr_cpu_core_realize_child, errp);
}

/*
 * This creates the CPU threads for a given @core.
 *
 * In order to create the threads, we need two inputs - number of
 * threads and the cpu_model. These are set as core object's properties.
 * When both of them become available/set, this routine will be called from
 * either property's set handler to create the threads.
 *
 * TODO: Dependence of threads creation on two properties is resulting
 * in this not-so-clean way of creating threads from either of the
 * property setters based on the order in which they get set. Check if
 * this can be handled in a better manner.
 */
static void spapr_cpu_core_create_threads(sPAPRCPUCore *core, Error **errp)
{
    int i;
    Error *local_err = NULL;

    for (i = 0; i < core->nr_threads; i++) {
        char id[32];

        object_initialize(&core->threads[i], sizeof(core->threads[i]),
                          object_class_get_name(core->oc));
        snprintf(id, sizeof(id), "thread[%d]", i);
        object_property_add_child(OBJECT(core), id, OBJECT(&core->threads[i]),
                                  &local_err);
        if (local_err) {
            goto err;
        }
    }
    return;

err:
    while (--i) {
        object_unparent(OBJECT(&core->threads[i]));
    }
    error_propagate(errp, local_err);
}

static char *spapr_cpu_core_prop_get_cpu_model(Object *obj, Error **errp)
{
    sPAPRCPUCore *core = SPAPR_CPU_CORE(obj);

    return g_strdup(object_class_get_name(core->oc));
}

static void spapr_cpu_core_prop_set_cpu_model(Object *obj, const char *val,
                                              Error **errp)
{
    sPAPRCPUCore *core = SPAPR_CPU_CORE(obj);
    MachineState *machine = MACHINE(qdev_get_machine());
    ObjectClass *oc = cpu_class_by_name(TYPE_POWERPC_CPU, val);
    ObjectClass *oc_base = cpu_class_by_name(TYPE_POWERPC_CPU,
                                             machine->cpu_model);
    if (!oc) {
        error_setg(errp, "Unknown CPU model %s", val);
        return;
    }

    /*
     * Currently cpu_model can't be different from what is specified with -cpu
     */
    if (strcmp(object_class_get_name(oc), object_class_get_name(oc_base))) {
        error_setg(errp, "cpu_model must be %s", machine->cpu_model);
        return;
    }

    core->oc = oc;
    if (core->nr_threads && core->oc) {
        spapr_cpu_core_create_threads(core, errp);
    }
}

static void spapr_cpu_core_prop_get_nr_threads(Object *obj, Visitor *v,
                                               const char *name, void *opaque,
                                               Error **errp)
{
    sPAPRCPUCore *core = SPAPR_CPU_CORE(obj);
    int64_t value = core->nr_threads;

    visit_type_int(v, name, &value, errp);
}

static void spapr_cpu_core_prop_set_nr_threads(Object *obj, Visitor *v,
                                               const char *name, void *opaque,
                                               Error **errp)
{
    sPAPRCPUCore *core = SPAPR_CPU_CORE(obj);
    Error *local_err = NULL;
    int64_t value;

    visit_type_int(v, name, &value, &local_err);
    if (local_err) {
        error_propagate(errp, local_err);
        return;
    }

    /* Allow only homogeneous configuration */
    if (value != smp_threads) {
        error_setg(errp, "nr_threads must be %d", smp_threads);
        return;
    }

    core->nr_threads = value;
    core->threads = g_new0(PowerPCCPU, core->nr_threads);

    if (core->nr_threads && core->oc) {
        spapr_cpu_core_create_threads(core, errp);
    }
}

static void spapr_cpu_core_instance_init(Object *obj)
{
    object_property_add(obj, "nr_threads", "int",
                        spapr_cpu_core_prop_get_nr_threads,
                        spapr_cpu_core_prop_set_nr_threads,
                        NULL, NULL, NULL);
    object_property_add_str(obj, "cpu_model",
                            spapr_cpu_core_prop_get_cpu_model,
                            spapr_cpu_core_prop_set_cpu_model,
                            NULL);
}

static void spapr_cpu_core_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->realize = spapr_cpu_core_realize;
}

static const TypeInfo spapr_cpu_core_type_info = {
    .name = TYPE_SPAPR_CPU_CORE,
    .parent = TYPE_CPU_CORE,
    .instance_init = spapr_cpu_core_instance_init,
    .instance_size = sizeof(sPAPRCPUCore),
    .class_init = spapr_cpu_core_class_init,
};

static void spapr_cpu_core_register_types(void)
{
    type_register_static(&spapr_cpu_core_type_info);
}

type_init(spapr_cpu_core_register_types)
