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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "ufo-ir-method-task.h"
#include <ufo/ufo.h>

// Private methods definitions
// Class related methods
static void ufo_ir_method_task_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ufo_ir_method_task_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ufo_ir_method_task_dispose (GObject *object);

// UfoTask Interface related methods
static void ufo_task_interface_init (UfoTaskIface *iface);
static void ufo_ir_method_task_setup (UfoTask *task, UfoResources *resources, GError **error);
static guint ufo_ir_method_task_get_num_inputs (UfoTask *task);
static guint ufo_ir_method_task_get_num_dimensions (UfoTask *task, guint input);
static UfoTaskMode ufo_ir_method_task_get_mode (UfoTask *task);
static void ufo_ir_method_task_get_requisition (UfoTask *task, UfoBuffer **inputs, UfoRequisition *requisition, GError **error);

// IrMethod private Methods

static const gchar *ufo_ir_method_task_get_package_name (UfoTaskNode *self);

G_DEFINE_TYPE_WITH_CODE (UfoIrMethodTask, ufo_ir_method_task, UFO_TYPE_TASK_NODE,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK, ufo_task_interface_init))

#define UFO_IR_METHOD_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_METHOD_TASK, UfoIrMethodTaskPrivate))

struct _UfoIrMethodTaskPrivate {
    UfoIrProjectorTask *projector;
    guint iterations_number;
};

enum {
    PROP_0,
    PROP_PROJECTOR,
    PROP_ITERATIONS_NUMBER,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = {NULL, };

static void
ufo_ir_method_task_class_init (UfoIrMethodTaskClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->set_property = ufo_ir_method_task_set_property;
    gobject_class->get_property = ufo_ir_method_task_get_property;
    gobject_class->dispose      = ufo_ir_method_task_dispose;

    properties[PROP_ITERATIONS_NUMBER] =
            g_param_spec_uint("num-iterations",
                              "Number of iterations of method",
                              "Number of iterations of method",
                              1, G_MAXUINT, 10,
                              G_PARAM_READWRITE);
    properties[PROP_PROJECTOR] =
            g_param_spec_object("projector",
                                "Current projector",
                                "Current projector",
                                UFO_IR_TYPE_PROJECTOR_TASK,
                                G_PARAM_READWRITE);
    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++){
        g_object_class_install_property (gobject_class, i, properties[i]);
    }

    g_type_class_add_private (gobject_class, sizeof(UfoIrMethodTaskPrivate));

    UfoTaskNodeClass *taskklass = UFO_TASK_NODE_CLASS (klass);
    taskklass->get_package_name = ufo_ir_method_task_get_package_name;
}

static void
ufo_task_interface_init (UfoTaskIface *iface)
{
    iface->setup = ufo_ir_method_task_setup;
    iface->get_num_inputs = ufo_ir_method_task_get_num_inputs;
    iface->get_num_dimensions = ufo_ir_method_task_get_num_dimensions;
    iface->get_mode = ufo_ir_method_task_get_mode;
    iface->get_requisition = ufo_ir_method_task_get_requisition;
}

static void
ufo_ir_method_task_init(UfoIrMethodTask *self)
{
    self->priv = UFO_IR_METHOD_TASK_GET_PRIVATE(self);
    self->priv->projector = NULL;
    self->priv->iterations_number = 10;
}

static void
ufo_ir_method_task_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
    UfoIrMethodTask *self = UFO_IR_METHOD_TASK (object);

    switch (property_id) {
        case PROP_ITERATIONS_NUMBER:
            ufo_ir_method_task_set_iterations_number(self, g_value_get_uint(value));
            break;
        case PROP_PROJECTOR:
            ufo_ir_method_task_set_projector(self, UFO_IR_PROJECTOR_TASK(g_value_get_object(value)));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_method_task_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
    UfoIrMethodTask *self = UFO_IR_METHOD_TASK (object);

    switch (property_id) {
        case PROP_ITERATIONS_NUMBER:
            g_value_set_uint(value, ufo_ir_method_task_get_iterations_number(self));
            break;
        case PROP_PROJECTOR:
            g_value_set_object(value, ufo_ir_method_task_get_projector(self));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

UfoIrProjectorTask *
ufo_ir_method_task_get_projector(UfoIrMethodTask *self)
{
    UfoIrMethodTaskPrivate *priv = UFO_IR_METHOD_TASK_GET_PRIVATE (self);
    return priv->projector;
}

void
ufo_ir_method_task_set_projector (UfoIrMethodTask *self, UfoIrProjectorTask *value)
{
    UfoIrMethodTaskPrivate *priv = UFO_IR_METHOD_TASK_GET_PRIVATE (self);

    if (priv->projector != NULL)
        g_object_unref (priv->projector);

    priv->projector = g_object_ref_sink (value);
}

guint
ufo_ir_method_task_get_iterations_number (UfoIrMethodTask *self)
{
    UfoIrMethodTaskPrivate *priv = UFO_IR_METHOD_TASK_GET_PRIVATE (self);
    return priv->iterations_number;
}

void
ufo_ir_method_task_set_iterations_number (UfoIrMethodTask *self, guint value)
{
    UfoIrMethodTaskPrivate *priv = UFO_IR_METHOD_TASK_GET_PRIVATE (self);
    priv->iterations_number = value;
}

static void
ufo_ir_method_task_dispose (GObject *object)
{
    UfoIrMethodTaskPrivate *priv = UFO_IR_METHOD_TASK_GET_PRIVATE (object);

    if (priv->projector != NULL) {
        g_object_unref (priv->projector);
        priv->projector = NULL;
    }

    G_OBJECT_CLASS (ufo_ir_method_task_parent_class)->dispose (object);
}

UfoNode *
ufo_ir_method_task_new (void)
{
    return UFO_NODE (g_object_new (UFO_IR_TYPE_METHOD_TASK, NULL));
}

static void
ufo_ir_method_task_setup (UfoTask      *task,
                          UfoResources *resources,
                          GError       **error)
{
    UfoIrMethodTaskPrivate *priv = UFO_IR_METHOD_TASK_GET_PRIVATE (task);

    if (priv->projector == NULL)
        g_set_error (error, UFO_TASK_ERROR, UFO_TASK_ERROR_SETUP, "No projector specified.");
    else
        ufo_task_setup (UFO_TASK (priv->projector), resources, error);
}

static guint
ufo_ir_method_task_get_num_inputs (UfoTask *task)
{
    return 1;
}

static guint
ufo_ir_method_task_get_num_dimensions (UfoTask *task,
                                            guint   input)
{
    g_return_val_if_fail (input == 0, 0);
    return 2;
}

static UfoTaskMode
ufo_ir_method_task_get_mode (UfoTask *task)
{
    return UFO_TASK_MODE_PROCESSOR | UFO_TASK_MODE_GPU;
}

static void
ufo_ir_method_task_get_requisition (UfoTask        *task,
                                    UfoBuffer      **inputs,
                                    UfoRequisition *requisition,
                                    GError        **error)
{
    UfoIrMethodTaskPrivate *priv = UFO_IR_METHOD_TASK_GET_PRIVATE (task);

    if (priv->projector == NULL)
        g_error ("Projector not specified");
    else
        ufo_task_get_requisition (UFO_TASK(priv->projector), inputs, requisition, error);
}

static const gchar *
ufo_ir_method_task_get_package_name (UfoTaskNode *self)
{
    return "ir";
}
