/*
 * sPAPR CPU core device.
 *
 * Copyright (C) 2016 Bharata B Rao <bharata@linux.vnet.ibm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */
#ifndef HW_SPAPR_CPU_CORE_H
#define HW_SPAPR_CPU_CORE_H

#include "hw/qdev.h"
#include "hw/cpu/core.h"

#define TYPE_SPAPR_CPU_CORE "spapr-cpu-core"
#define SPAPR_CPU_CORE(obj) \
    OBJECT_CHECK(sPAPRCPUCore, (obj), TYPE_SPAPR_CPU_CORE)

typedef struct sPAPRCPUCore {
    /*< private >*/
    CPUCore parent_obj;

    /*< public >*/
    int nr_threads;
    ObjectClass *oc;
    PowerPCCPU *threads;
} sPAPRCPUCore;

#endif
