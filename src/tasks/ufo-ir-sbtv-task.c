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

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "ufo-ir-splitbregman-task.h"

static void ufo_ir_splitbregman_task_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ufo_ir_splitbregman_task_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ufo_ir_splitbregman_task_setup (UfoTask *task, UfoResources *resources, GError **error);
static void ufo_ir_splitbregman_task_finalize (GObject *object);
static gboolean ufo_ir_splitbregman_task_process (UfoTask *task, UfoBuffer **inputs, UfoBuffer *output, UfoRequisition *requisition);
static const gchar *ufo_ir_splitbregman_task_get_package_name(UfoTaskNode *self);

struct _UfoIrSplitBregmanTaskPrivate {
    // Method parameters
    gfloat mu;
    gfloat lambda;

    UfoIrStateDependentTask *sp_domain;
};

static void ufo_task_interface_init (UfoTaskIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrSplitBregmanTask, ufo_ir_splitbregman_task, UFO_IR_TYPE_METHOD_TASK,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_IR_SPLITBREGMAN_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_SPLITBREGMAN_TASK, UfoIrSplitBregmanTaskPrivate))

enum {
    PROP_0,
    PROP_MU,
    PROP_LAMBDA,
    PROP_SP_DOMAIN,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

// -----------------------------------------------------------------------------
// Init methods
// -----------------------------------------------------------------------------
static void
ufo_task_interface_init (UfoTaskIface *iface)
{
    iface->setup = ufo_ir_splitbregman_task_setup;
    iface->process = ufo_ir_splitbregman_task_process;
}

static void
ufo_ir_splitbregman_task_class_init (UfoIrSplitBregmanTaskClass *klass)
{
    GObjectClass *oclass = G_OBJECT_CLASS (klass);

    oclass->set_property = ufo_ir_splitbregman_task_set_property;
    oclass->get_property = ufo_ir_splitbregman_task_get_property;
    oclass->finalize = ufo_ir_splitbregman_task_finalize;

    UfoTaskNodeClass * tnclass= UFO_TASK_NODE_CLASS(klass);
    tnclass->get_package_name = ufo_ir_splitbregman_task_get_package_name;

    properties[PROP_LAMBDA] =
            g_param_spec_float("lambda",
                               "Lambda",
                               "Lambda",
                               0.0f, G_MAXFLOAT, 0.1f,
                               G_PARAM_READWRITE);
    properties[PROP_LAMBDA] =
            g_param_spec_float("mu",
                               "Mu",
                               "Mu",
                               0.0f, G_MAXFLOAT, 0.5f,
                               G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (oclass, i, properties[i]);

    g_type_class_add_private (oclass, sizeof(UfoIrSplitBregmanTaskPrivate));
}

static void
ufo_ir_splitbregman_task_init(UfoIrSplitBregmanTask *self)
{
    self->priv = UFO_IR_SPLITBREGMAN_TASK_GET_PRIVATE(self);
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Getters and setters
// -----------------------------------------------------------------------------
gfloat ufo_ir_splitbregman_task_get_mu(UfoIrSplitBregmanTask *self) {
    UfoIrSplitBregmanTaskPrivate *priv = UFO_IR_SPLITBREGMAN_TASK_GET_PRIVATE (self);
    return priv->mu;
}

void   ufo_ir_splitbregman_task_set_mu(UfoIrSplitBregmanTask *self, gfloat value) {
    UfoIrSplitBregmanTaskPrivate *priv = UFO_IR_SPLITBREGMAN_TASK_GET_PRIVATE (self);
    priv->mu = value;
}

gfloat ufo_ir_splitbregman_task_get_lambda(UfoIrSplitBregmanTask *self) {
    UfoIrSplitBregmanTaskPrivate *priv = UFO_IR_SPLITBREGMAN_TASK_GET_PRIVATE (self);
    return priv->lambda;
}

void   ufo_ir_splitbregman_task_set_lambda(UfoIrSplitBregmanTask *self, gfloat value) {
    UfoIrSplitBregmanTaskPrivate *priv = UFO_IR_SPLITBREGMAN_TASK_GET_PRIVATE (self);
    priv->lambda = value;
}

UfoIrStateDependentTask *ufo_ir_splitbregman_task_get_sp_domain(UfoIrSplitBregmanTask *self) {
    UfoIrSplitBregmanTaskPrivate *priv = UFO_IR_SPLITBREGMAN_TASK_GET_PRIVATE (self);
    return priv->sp_domain;
}

void ufo_ir_splitbregman_task_set_sp_domain(UfoIrSplitBregmanTask *self, UfoIrStateDependentTask *value) {
    UfoIrSplitBregmanTaskPrivate *priv = UFO_IR_SPLITBREGMAN_TASK_GET_PRIVATE (self);
    priv->sp_domain = value;
}

static void
ufo_ir_splitbregman_task_set_property (GObject *object,
                                       guint property_id,
                                       const GValue *value,
                                       GParamSpec *pspec) {
    UfoIrSplitBregmanTask *self = UFO_IR_SPLITBREGMAN_TASK (object);

    switch (property_id) {
        case PROP_MU:
            ufo_ir_splitbregman_task_set_mu(self, g_value_get_float(value));
            break;
        case PROP_LAMBDA:
            ufo_ir_splitbregman_task_set_lambda(self, g_value_get_float(value));
            break;
        case PROP_SP_DOMAIN:
            ufo_ir_splitbregman_task_set_sp_domain(self, g_value_get_object(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_splitbregman_task_get_property (GObject *object,
                                       guint property_id,
                                       GValue *value,
                                       GParamSpec *pspec) {
    UfoIrSplitBregmanTask *self = UFO_IR_SPLITBREGMAN_TASK (object);

    switch (property_id) {
        case PROP_MU:
            g_value_set_float(value, ufo_ir_splitbregman_task_get_mu(self));
            break;
        case PROP_LAMBDA:
            g_value_set_float(value, ufo_ir_splitbregman_task_get_lambda(self));
            break;
        case PROP_SP_DOMAIN:
            g_value_set_object(value, ufo_ir_splitbregman_task_get_sp_domain(self));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Task interface realization
// -----------------------------------------------------------------------------

UfoNode *
ufo_ir_splitbregman_task_new (void)
{
    return UFO_NODE (g_object_new (UFO_IR_TYPE_SPLITBREGMAN_TASK, NULL));
}

static void
ufo_ir_splitbregman_task_setup (UfoTask *task,
                       UfoResources *resources,
                       GError **error)
{
}

static gboolean
ufo_ir_splitbregman_task_process (UfoTask *task,
                         UfoBuffer **inputs,
                         UfoBuffer *output,
                         UfoRequisition *requisition)
{
    return TRUE;
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// GObject methods
// -----------------------------------------------------------------------------
static void
ufo_ir_splitbregman_task_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_ir_splitbregman_task_parent_class)->finalize (object);
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// TaskNode methods
// -----------------------------------------------------------------------------
static const gchar *ufo_ir_splitbregman_task_get_package_name(UfoTaskNode *self) {
    return g_strdup("ir");
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------------
static void calculate_b(UfoBuffer *fbp, UfoBuffer *dx, UfoBuffer *dy, UfoBuffer *bx, UfoBuffer *by, UfoBuffer *b, UfoMethod *method, gpointer cmd_queue, UfoResources *resources)
{
    UfoBuffer *tmpx = ufo_buffer_dup(fbp);
    UfoBuffer *tmpy = ufo_buffer_dup(fbp);
    UfoBuffer *tmpDif = ufo_buffer_dup(fbp);

    // tmpx = DXT(dx - bx);
    ufo_op_deduction(dx, bx, tmpDif, resources, cmd_queue);
    dxt_op(method, tmpDif, tmpx, cmd_queue);

    // tmpx = DYT(dy - by);
    ufo_op_deduction(dy, by, tmpDif, resources, cmd_queue);
    dyt_op(method, tmpDif, tmpy, cmd_queue);

    // b = mu * At(f) + lambda * (tmpx + tmpy)
    ufo_op_add(tmpx, tmpy, b, resources, cmd_queue);
    mult(b, LAMBDA, cmd_queue);
    ufo_op_add(fbp, b, b, resources, cmd_queue);

    g_object_unref(tmpx);
    g_object_unref(tmpy);
    g_object_unref(tmpDif);
}
// -----------------------------------------------------------------------------
