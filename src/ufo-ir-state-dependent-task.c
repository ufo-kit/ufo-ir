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

#include "ufo-ir-state-dependent-task.h"



struct _UfoIrStateDependentTaskPrivate {
    gboolean is_forward;
};

// Private methods definitions
static void ufo_task_interface_init (UfoTaskIface *iface);
static gboolean ufo_ir_state_dependent_task_process (UfoTask *self, UfoBuffer **inputs, UfoBuffer *output, UfoRequisition *requisition);
//static UfoTaskMode ufo_ir_state_dependent_task_get_mode (UfoTask *task);
//static guint ufo_ir_state_dependent_task_get_num_dimensions (UfoTask *task, guint input);
//static guint ufo_ir_state_dependent_task_get_num_inputs (UfoTask *task);
//static void ufo_ir_state_dependent_task_get_requisition (UfoTask *task, UfoBuffer **inputs, UfoRequisition *requisition);
static void ufo_ir_state_dependent_task_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ufo_ir_state_dependent_task_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ufo_ir_state_dependent_task_finalize (GObject *object);
static const gchar *ufo_ir_state_dependent_task_get_package_name (UfoTaskNode *node);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (UfoIrStateDependentTask, ufo_ir_state_dependent_task, UFO_TYPE_TASK_NODE,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_IR_STATE_DEPENDENT_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_STATE_DEPENDENT_TASK, UfoIrStateDependentTaskPrivate))

enum {
    PROP_0,
    PROP_IS_FORWARD,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

// -----------------------------------------------------------------------------
// Init methods
// -----------------------------------------------------------------------------
static void
ufo_task_interface_init (UfoTaskIface *iface)
{
    iface->process = ufo_ir_state_dependent_task_process;
}

static void
ufo_ir_state_dependent_task_class_init (UfoIrStateDependentTaskClass *klass)
{
    GObjectClass *oclass = G_OBJECT_CLASS (klass);

    oclass->set_property = ufo_ir_state_dependent_task_set_property;
    oclass->get_property = ufo_ir_state_dependent_task_get_property;
    oclass->finalize = ufo_ir_state_dependent_task_finalize;

    UfoTaskNodeClass *tclass = UFO_TASK_NODE_CLASS(klass);
    tclass->get_package_name = ufo_ir_state_dependent_task_get_package_name;

    properties[PROP_IS_FORWARD] =
        g_param_spec_boolean ("is_forward",
                             "Indicates process direction",
                             "Indicates process direction",
                              TRUE,
                              G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (oclass, i, properties[i]);

    g_type_class_add_private (oclass, sizeof(UfoIrStateDependentTaskPrivate));

    klass->backward = NULL;
    klass->forward = NULL;
    g_print("qwe");
}

static void
ufo_ir_state_dependent_task_init(UfoIrStateDependentTask *self)
{
    self->priv = UFO_IR_STATE_DEPENDENT_TASK_GET_PRIVATE(self);
    self->priv->is_forward = TRUE;
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Public methods
// -----------------------------------------------------------------------------
gboolean ufo_ir_state_dependent_task_forward (UfoIrStateDependentTask        *task,
                                              UfoBuffer     **inputs,
                                              UfoBuffer      *output,
                                              UfoRequisition *requisition){
    g_return_if_fail(UFO_IR_IS_STATE_DEPENDENT_TASK(task));

    if(UFO_IR_STATE_DEPENDENT_TASK_CLASS(task)->forward != NULL){
        UFO_IR_STATE_DEPENDENT_TASK_CLASS(task)->forward(UFO_IR_STATE_DEPENDENT_TASK(task), inputs, output, requisition);
    }
    else{
        g_warning ("Class '%s' does not override the mandatory "
                   "UfoIrStateDependentTask.Forward() virtual function.",
                   G_OBJECT_TYPE_NAME (task));
    }

}

gboolean ufo_ir_state_dependent_task_backward (UfoIrStateDependentTask *task,
                                               UfoBuffer     **inputs,
                                               UfoBuffer      *output,
                                               UfoRequisition *requisition){
    g_return_if_fail(UFO_IR_IS_STATE_DEPENDENT_TASK(task));

    UfoIrStateDependentTaskClass *klass = NULL;
    klass = UFO_IR_STATE_DEPENDENT_TASK_CLASS(task);

    if(klass->backward != NULL){
        UFO_IR_STATE_DEPENDENT_TASK_CLASS(task)->backward(UFO_IR_STATE_DEPENDENT_TASK(task), inputs, output, requisition);
    }
    else{
        g_warning ("Class '%s' does not override the mandatory "
                   "UfoIrStateDependentTask.Backward() virtual function.",
                   G_OBJECT_TYPE_NAME (task));
    }
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Finalizataion
// -----------------------------------------------------------------------------
static void
ufo_ir_state_dependent_task_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_ir_state_dependent_task_parent_class)->finalize (object);
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Getters and setters
// -----------------------------------------------------------------------------
static void
ufo_ir_state_dependent_task_set_property (GObject *object,
                              guint property_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
    UfoIrStateDependentTaskPrivate *priv = UFO_IR_STATE_DEPENDENT_TASK_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_IS_FORWARD:
            priv->is_forward = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_state_dependent_task_get_property (GObject *object,
                              guint property_id,
                              GValue *value,
                              GParamSpec *pspec)
{
    UfoIrStateDependentTaskPrivate *priv = UFO_IR_STATE_DEPENDENT_TASK_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_IS_FORWARD:
            g_value_set_boolean(value, priv->is_forward);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ITaskNode implementation
// -----------------------------------------------------------------------------
UfoNode *
ufo_ir_state_dependent_task_new (void)
{
    return UFO_NODE (g_object_new (UFO_IR_TYPE_STATE_DEPENDENT_TASK, NULL));
}

static gboolean
ufo_ir_state_dependent_task_process (UfoTask *self,
                         UfoBuffer **inputs,
                         UfoBuffer *output,
                         UfoRequisition *requisition)
{
    UfoIrStateDependentTaskPrivate * priv= UFO_IR_STATE_DEPENDENT_TASK_GET_PRIVATE(self);

    if(priv->is_forward)
    {
        return ufo_ir_state_dependent_task_forward(UFO_IR_STATE_DEPENDENT_TASK(self), inputs, output, requisition);
    }
    else
    {
        return ufo_ir_state_dependent_task_backward(UFO_IR_STATE_DEPENDENT_TASK(self), inputs, output, requisition);
    }
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// TaskNode implementation
// -----------------------------------------------------------------------------
static const gchar *ufo_ir_state_dependent_task_get_package_name (UfoTaskNode *node)
{
    return g_strdup ("ir");
}

// -----------------------------------------------------------------------------
