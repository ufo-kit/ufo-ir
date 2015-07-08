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

#include "ufo-ir-projector-task.h"


struct _UfoIrProjectorTaskPrivate {
    gfloat axis_position;
    gfloat step;
    gfloat relaxation;
    gfloat correction_scale;
};

static void ufo_task_interface_init (UfoTaskIface *iface);
static guint ufo_ir_projector_task_get_num_inputs (UfoTask *task);
static guint ufo_ir_projector_task_get_num_dimensions (UfoTask *task, guint input);
static void ufo_ir_projector_task_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ufo_ir_projector_task_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ufo_ir_projector_task_finalize (GObject *object);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (UfoIrProjectorTask, ufo_ir_projector_task, UFO_IR_TYPE_STATE_DEPENDENT_TASK,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_IR_PROJECTOR_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_PROJECTOR_TASK, UfoIrProjectorTaskPrivate))

enum {
    PROP_0 = 100,
    PROP_AXIS_POSITION,
    PROP_STEP,
    PROP_RELAXATION,
    PROP_CORRECTION_SCALE,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

// -----------------------------------------------------------------------------
// Init methods
// -----------------------------------------------------------------------------

static void
ufo_task_interface_init (UfoTaskIface *iface)
{
    iface->get_num_inputs = ufo_ir_projector_task_get_num_inputs;
    iface->get_num_dimensions = ufo_ir_projector_task_get_num_dimensions;
}

static void
ufo_ir_projector_task_class_init (UfoIrProjectorTaskClass *klass)
{
    GObjectClass *oclass = G_OBJECT_CLASS (klass);

    oclass->set_property = ufo_ir_projector_task_set_property;
    oclass->get_property = ufo_ir_projector_task_get_property;
    oclass->finalize = ufo_ir_projector_task_finalize;

    properties[PROP_AXIS_POSITION] =
            g_param_spec_float ("axis_position",
                              "Axis position",
                              "Axis position",
                              -1, G_MAXFLOAT, -1,
                              G_PARAM_READWRITE);

    properties[PROP_STEP] =
            g_param_spec_float ("step",
                                "Projection step in RAD",
                                "Projection step in RAD",
                                0.0f, G_MAXFLOAT, 0.0f,
                                G_PARAM_READWRITE);

    properties[PROP_RELAXATION] =
            g_param_spec_float ("relaxation",
                                "Projection relaxation parameter",
                                "Projection relaxation parameter",
                                G_MINFLOAT, G_MAXFLOAT, 1.0f,
                                G_PARAM_READWRITE);
    properties[PROP_CORRECTION_SCALE] =
            g_param_spec_float ("correction_scale",
                                "FP correction scale",
                                "FP correction scale",
                                G_MINFLOAT, G_MAXFLOAT, 1.0f,
                                G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (oclass, i, properties[i]);

    g_type_class_add_private (oclass, sizeof(UfoIrProjectorTaskPrivate));
}

static void
ufo_ir_projector_task_init(UfoIrProjectorTask *self)
{
    UfoIrProjectorTaskPrivate *priv;
    self->priv = priv = UFO_IR_PROJECTOR_TASK_GET_PRIVATE(self);
    priv->axis_position = -1;
    priv->step = 0;
    priv->relaxation = 1;
    priv->correction_scale = 1;
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Getters and setters
// -----------------------------------------------------------------------------
static void
ufo_ir_projector_task_set_property (GObject *object,
                                    guint property_id,
                                    const GValue *value,
                                    GParamSpec *pspec) {
    UfoIrProjectorTask *self = UFO_IR_PROJECTOR_TASK (object);

    switch (property_id) {
        case PROP_AXIS_POSITION:
            ufo_ir_projector_task_set_axis_position(self, g_value_get_float (value));
            break;
        case PROP_STEP:
            ufo_ir_projector_task_set_step(self, g_value_get_float (value));
            break;
        case PROP_RELAXATION:
            ufo_ir_projector_task_set_relaxation(self, g_value_get_float (value));
            break;
        case PROP_CORRECTION_SCALE:
            ufo_ir_projector_task_set_correction_scale(self, g_value_get_float (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_projector_task_get_property (GObject *object,
                              guint property_id,
                              GValue *value,
                              GParamSpec *pspec) {
    UfoIrProjectorTask *self = UFO_IR_PROJECTOR_TASK (object);

    switch (property_id) {
        case PROP_AXIS_POSITION:
            g_value_set_float (value, ufo_ir_projector_task_get_axis_position(self));
            break;
        case PROP_STEP:
            g_value_set_float (value, ufo_ir_projector_task_get_step(self));
            break;
        case PROP_RELAXATION:
            g_value_set_float (value, ufo_ir_projector_task_get_relaxation(self));
            break;
        case PROP_CORRECTION_SCALE:
            g_value_set_float (value, ufo_ir_projector_task_get_correction_scale(self));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

gfloat ufo_ir_projector_task_get_step(UfoIrProjectorTask *self) {
    UfoIrProjectorTaskPrivate *priv = UFO_IR_PROJECTOR_TASK_GET_PRIVATE (self);
    return priv->step;
}

void ufo_ir_projector_task_set_step(UfoIrProjectorTask *self, gfloat value) {
    UfoIrProjectorTaskPrivate *priv = UFO_IR_PROJECTOR_TASK_GET_PRIVATE (self);
    priv->step = value;
}

gfloat ufo_ir_projector_task_get_axis_position(UfoIrProjectorTask *self) {
    UfoIrProjectorTaskPrivate *priv = UFO_IR_PROJECTOR_TASK_GET_PRIVATE (self);
    return priv->axis_position;
}

void ufo_ir_projector_task_set_axis_position(UfoIrProjectorTask *self, gfloat value) {
    UfoIrProjectorTaskPrivate *priv = UFO_IR_PROJECTOR_TASK_GET_PRIVATE (self);
    priv->axis_position = value;
}

gfloat ufo_ir_projector_task_get_relaxation(UfoIrProjectorTask *self) {
    UfoIrProjectorTaskPrivate *priv = UFO_IR_PROJECTOR_TASK_GET_PRIVATE (self);
    return priv->relaxation;
}

void ufo_ir_projector_task_set_relaxation(UfoIrProjectorTask *self, gfloat value) {
    UfoIrProjectorTaskPrivate *priv = UFO_IR_PROJECTOR_TASK_GET_PRIVATE (self);
    priv->relaxation = value;
}

gfloat ufo_ir_projector_task_get_correction_scale(UfoIrProjectorTask *self) {
    UfoIrProjectorTaskPrivate *priv = UFO_IR_PROJECTOR_TASK_GET_PRIVATE (self);
    return priv->correction_scale;
}

void   ufo_ir_projector_task_set_correction_scale(UfoIrProjectorTask *self, gfloat value) {
    UfoIrProjectorTaskPrivate *priv = UFO_IR_PROJECTOR_TASK_GET_PRIVATE (self);
    priv->correction_scale = value;
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Finalizataion
// -----------------------------------------------------------------------------
static void
ufo_ir_projector_task_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_ir_projector_task_parent_class)->finalize (object);
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ITaskNode implementation
// -----------------------------------------------------------------------------

static guint
ufo_ir_projector_task_get_num_inputs (UfoTask *task)
{
    return 1;
}

static guint
ufo_ir_projector_task_get_num_dimensions (UfoTask *task,
                                             guint input)
{
    return 2;
}

UfoNode *
ufo_ir_projector_task_new (void)
{
    return UFO_NODE (g_object_new (UFO_IR_TYPE_PROJECTOR_TASK, NULL));
}

// -----------------------------------------------------------------------------

