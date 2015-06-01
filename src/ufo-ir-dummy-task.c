/*
 * Copyright (C) 2011-2013 Karlsruhe Institute of Technology
 *
 * This file is part of Ufo.
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "ufo-ir-dummy-task.h"
#include <gmodule.h>
#include <ufo/ufo.h>

/**
 * SECTION:ufo-copy-task
 * @Short_description: Copy data from input to output
 * @Title: UfoCopyTask
 *
 * Copies input to output. This is useful in combination with a
 * #UfoFixedScheduler in order to emulate broadcasting behaviour.
 */

static void ufo_task_interface_init (UfoTaskIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrDummyTask, ufo_ir_dummy_task, UFO_TYPE_TASK_NODE,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

enum {
    PROP_0,
    N_PROPERTIES
};

UfoNode *
ufo_ir_dummy_task_new (void)
{
    return UFO_NODE (g_object_new (UFO_IR_TYPE_DUMMY_TASK, NULL));
}

static void
ufo_ir_dummy_task_setup (UfoTask *task,
                         UfoResources *resources,
                         GError **error)
{
}

static guint
ufo_ir_dummy_task_get_num_inputs (UfoTask *task)
{
    return 1;
}

static guint
ufo_ir_dummy_task_get_num_dimensions (UfoTask *task,
                                  guint input)
{
    return -1;
}

static UfoTaskMode
ufo_ir_dummy_task_get_mode (UfoTask *task)
{
    return UFO_TASK_MODE_PROCESSOR | UFO_TASK_MODE_CPU;
}

static void
ufo_ir_dummy_task_get_requisition (UfoTask *task,
                               UfoBuffer **inputs,
                               UfoRequisition *requisition)
{
    ufo_buffer_get_requisition (inputs[0], requisition);
}

static gboolean
ufo_ir_dummy_task_process (UfoTask *task,
                       UfoBuffer **inputs,
                       UfoBuffer *output,
                       UfoRequisition *requisition)
{
    ufo_buffer_copy (inputs[0], output);
    return TRUE;
}

static const gchar *
ufo_ir_dummy_task_get_package_name_real (UfoTaskNode *task_node)
{
    return "ir";
}

static void
ufo_task_interface_init (UfoTaskIface *iface)
{
    iface->setup = ufo_ir_dummy_task_setup;
    iface->get_num_inputs = ufo_ir_dummy_task_get_num_inputs;
    iface->get_num_dimensions = ufo_ir_dummy_task_get_num_dimensions;
    iface->get_mode = ufo_ir_dummy_task_get_mode;
    iface->get_requisition = ufo_ir_dummy_task_get_requisition;
    iface->process = ufo_ir_dummy_task_process;
}

static void
ufo_ir_dummy_task_class_init (UfoIrDummyTaskClass *klass)
{
    UfoTaskNodeClass *taskklass = UFO_TASK_NODE_GET_CLASS(klass);
    taskklass->get_package_name = ufo_ir_dummy_task_get_package_name_real;
}


static void
ufo_ir_dummy_task_init (UfoIrDummyTask *task)
{
    ufo_task_node_set_plugin_name (UFO_TASK_NODE (task), "broadcast-task");
}


