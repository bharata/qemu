/*
 * PPC CPU socket abstraction
 *
 * Copyright (c) 2015 SUSE Linux GmbH
 * Copyright (C) 2015 Bharata B Rao <bharata@linux.vnet.ibm.com>
 */

#include "hw/qdev.h"
#include "hw/ppc/cpu-socket.h"
#include "sysemu/cpus.h"

static int ppc_cpu_socket_realize_child(Object *child, void *opaque)
{
    Error **errp = opaque;

    object_property_set_bool(child, true, "realized", errp);
    if (*errp) {
        return 1;
    } else {
        return 0;
    }
}

static void ppc_cpu_socket_realize(DeviceState *dev, Error **errp)
{
    object_child_foreach(OBJECT(dev), ppc_cpu_socket_realize_child, errp);
}

static void ppc_cpu_socket_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->realize = ppc_cpu_socket_realize;
}

static const TypeInfo ppc_cpu_socket_type_info = {
    .name = TYPE_POWERPC_CPU_SOCKET,
    .parent = TYPE_CPU_SOCKET,
    .class_init = ppc_cpu_socket_class_init,
};

static void ppc_cpu_socket_register_types(void)
{
    type_register_static(&ppc_cpu_socket_type_info);
}

type_init(ppc_cpu_socket_register_types)
