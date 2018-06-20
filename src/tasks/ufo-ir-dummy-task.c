/*
 * Copyright (C) 2011-2015 Karlsruhe Institute of Technology
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

#include "ufo-ir-dummy-task.h"
#include <ufo/ufo.h>

struct _UfoIrDummyTaskPrivate {
    UfoTaskNode *subtask;
};

static void ufo_task_interface_init (UfoTaskIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrDummyTask, ufo_ir_dummy_task, UFO_TYPE_TASK_NODE,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_IR_DUMMY_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_DUMMY_TASK, UfoIrDummyTaskPrivate))

enum {
    PROP_0,
    PROP_SUBTASK,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

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

static void
ufo_ir_dummy_task_get_requisition (UfoTask *task,
                                   UfoBuffer **inputs,
                                   UfoRequisition *requisition,
                                   GError **error)
{
    requisition->n_dims = 2;
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
    return 2;
}

static const gchar*
get_package_name(UfoTaskNode *self)
{
    return g_strdup("ir");
}

static UfoTaskMode
ufo_ir_dummy_task_get_mode (UfoTask *task)
{
    return UFO_TASK_MODE_PROCESSOR;
}

static gboolean
ufo_ir_dummy_task_process (UfoTask *task,
                         UfoBuffer **inputs,
                         UfoBuffer *output,
                         UfoRequisition *requisition)
{
    ufo_buffer_copy(inputs[0], output);
    return TRUE;
}

static void
ufo_ir_dummy_task_set_property (GObject *object,
                              guint property_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
    UfoIrDummyTaskPrivate *priv = UFO_IR_DUMMY_TASK_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_SUBTASK:
            priv->subtask = g_value_get_object(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_dummy_task_get_property (GObject *object,
                              guint property_id,
                              GValue *value,
                              GParamSpec *pspec)
{
    UfoIrDummyTaskPrivate *priv = UFO_IR_DUMMY_TASK_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_SUBTASK:
            g_value_set_object(value, priv->subtask);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_dummy_task_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_ir_dummy_task_parent_class)->finalize (object);
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
    GObjectClass *oclass = G_OBJECT_CLASS (klass);

    oclass->set_property = ufo_ir_dummy_task_set_property;
    oclass->get_property = ufo_ir_dummy_task_get_property;
    oclass->finalize = ufo_ir_dummy_task_finalize;

    UfoTaskNodeClass *tnclass = UFO_TASK_NODE_CLASS(klass);
    tnclass->get_package_name = get_package_name;

    properties[PROP_SUBTASK] =
            g_param_spec_object( "subtask",
                                 "subtask",
                                 "Subtask TaskNode",
                                 UFO_TYPE_TASK_NODE,
                                 G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (oclass, i, properties[i]);

    g_type_class_add_private (oclass, sizeof(UfoIrDummyTaskPrivate));
}

static void
ufo_ir_dummy_task_init(UfoIrDummyTask *self)
{
    self->priv = UFO_IR_DUMMY_TASK_GET_PRIVATE(self);
}
