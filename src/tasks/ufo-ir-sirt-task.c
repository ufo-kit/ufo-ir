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

#include "ufo-ir-sirt-task.h"
#include "core/ufo-ir-basic-ops.h"

static void ufo_ir_sirt_task_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ufo_ir_sirt_task_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ufo_task_interface_init (UfoTaskIface *iface);
static void ufo_ir_sirt_task_setup (UfoTask *task, UfoResources *resources, GError **error);
static gboolean ufo_ir_sirt_task_process (UfoTask *task, UfoBuffer **inputs, UfoBuffer *output, UfoRequisition *requisition);
static void ufo_ir_sirt_task_finalize (GObject *object);
static const gchar *ufo_ir_sirt_task_get_package_name(UfoTaskNode *self);

struct _UfoIrSirtTaskPrivate {
    gfloat relaxation_factor;
    gpointer op_set_kernel;
    gpointer op_inv_kernel;
    gpointer op_add_kernel;
    gpointer op_mul_kernel;
};

G_DEFINE_TYPE_WITH_CODE (UfoIrSirtTask, ufo_ir_sirt_task, UFO_IR_TYPE_METHOD_TASK,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_IR_SIRT_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_SIRT_TASK, UfoIrSirtTaskPrivate))

enum {
    PROP_0 = 100,
    PROP_RELAXATION_FACTOR,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

// -----------------------------------------------------------------------------
// Init methods
// -----------------------------------------------------------------------------

static void
ufo_task_interface_init (UfoTaskIface *iface) {
    iface->process = ufo_ir_sirt_task_process;
    iface->setup = ufo_ir_sirt_task_setup;
}

static void
ufo_ir_sirt_task_class_init (UfoIrSirtTaskClass *klass) {
    GObjectClass *oclass = G_OBJECT_CLASS (klass);

    oclass->set_property = ufo_ir_sirt_task_set_property;
    oclass->get_property = ufo_ir_sirt_task_get_property;
    oclass->finalize = ufo_ir_sirt_task_finalize;

    UfoTaskNodeClass * tnclass= UFO_TASK_NODE_CLASS(klass);
    tnclass->get_package_name = ufo_ir_sirt_task_get_package_name;

    properties[PROP_RELAXATION_FACTOR] =
            g_param_spec_float("relaxation_factor",
                               "relaxation_factor",
                               "Relaxation factor",
                               0.0f, 1.0f, 0.5f,
                               G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (oclass, i, properties[i]);

    g_type_class_add_private (oclass, sizeof(UfoIrSirtTaskPrivate));
}

static void
ufo_ir_sirt_task_init(UfoIrSirtTask *self) {
    self->priv = UFO_IR_SIRT_TASK_GET_PRIVATE(self);
    self->priv->relaxation_factor = 0.25;
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Finalization
// -----------------------------------------------------------------------------

static void
ufo_ir_sirt_task_finalize (GObject *object) {
    G_OBJECT_CLASS (ufo_ir_sirt_task_parent_class)->finalize (object);
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Getters and setters
// -----------------------------------------------------------------------------
static void
ufo_ir_sirt_task_set_property (GObject *object,
                               guint property_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
    UfoIrSirtTask *self = UFO_IR_SIRT_TASK (object);

    switch (property_id) {
        case PROP_RELAXATION_FACTOR:
            ufo_ir_sirt_task_set_relaxation_factor(self, g_value_get_float(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_sirt_task_get_property (GObject *object,
                               guint property_id,
                               GValue *value,
                               GParamSpec *pspec)
{
    UfoIrSirtTask *self = UFO_IR_SIRT_TASK (object);

    switch (property_id) {
        case PROP_RELAXATION_FACTOR:
            g_value_set_float(value, ufo_ir_sirt_task_get_relaxation_factor(self));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

gfloat
ufo_ir_sirt_task_get_relaxation_factor(UfoIrSirtTask *self) {
    UfoIrSirtTaskPrivate *priv = UFO_IR_SIRT_TASK_GET_PRIVATE (self);
    return priv->relaxation_factor;
}

void
ufo_ir_sirt_task_set_relaxation_factor(UfoIrSirtTask *self, gfloat value) {
    UfoIrSirtTaskPrivate *priv = UFO_IR_SIRT_TASK_GET_PRIVATE (self);
    priv->relaxation_factor = value;
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Task interface realization
// -----------------------------------------------------------------------------
UfoNode *
ufo_ir_sirt_task_new (void) {
    return UFO_NODE (g_object_new (UFO_IR_TYPE_SIRT_TASK, NULL));
}

static void
ufo_ir_sirt_task_setup (UfoTask      *task,
                        UfoResources *resources,
                        GError       **error)
{
    ufo_task_node_set_proc_node(UFO_TASK_NODE(ufo_ir_method_task_get_projector(UFO_IR_METHOD_TASK(task))), ufo_task_node_get_proc_node(UFO_TASK_NODE(task)));

    ufo_task_setup(UFO_TASK(ufo_ir_method_task_get_projector(UFO_IR_METHOD_TASK(task))), resources, error);
    UfoIrSirtTaskPrivate *priv = UFO_IR_SIRT_TASK_GET_PRIVATE (task);
    priv->op_set_kernel = ufo_ir_op_set_generate_kernel(resources);
    priv->op_inv_kernel = ufo_ir_op_inv_generate_kernel(resources);
    priv->op_add_kernel = ufo_ir_op_add_generate_kernel(resources);
    priv->op_mul_kernel = ufo_ir_op_mul_generate_kernel(resources);
}

static gboolean
ufo_ir_sirt_task_process (UfoTask *task,
                          UfoBuffer **inputs,
                          UfoBuffer *output,
                          UfoRequisition *requisition) {
    UfoIrSirtTaskPrivate *priv = UFO_IR_SIRT_TASK_GET_PRIVATE (task);
    UfoGpuNode *node = UFO_GPU_NODE (ufo_task_node_get_proc_node (UFO_TASK_NODE(task)));
    cl_command_queue cmd_queue = (cl_command_queue)ufo_gpu_node_get_cmd_queue (node);

//    GTimer *timer = g_timer_new ();
//    g_timer_reset(timer);
//    clFinish(cmd_queue);
//    g_timer_start(timer);

    // Get and setup projector
    UfoIrProjectorTask *projector = ufo_ir_method_task_get_projector(UFO_IR_METHOD_TASK(task));
    UfoIrStateDependentTask *sdprojector = UFO_IR_STATE_DEPENDENT_TASK(projector);
    ufo_ir_projector_task_set_relaxation(projector, 1.0f);
    ufo_ir_projector_task_set_correction_scale(projector, 1.0f);

    // calculate Ray waights
    UfoBuffer *volume_tmp = ufo_buffer_dup (output);
    ufo_ir_op_set (volume_tmp,  1.0f, cmd_queue, priv->op_set_kernel);
    UfoBuffer *ray_weights   = ufo_buffer_dup (inputs[0]);
    ufo_ir_op_set (ray_weights, 0.0f, cmd_queue, priv->op_set_kernel);
    ufo_ir_state_dependent_task_forward(sdprojector, &volume_tmp, ray_weights, requisition);
    ufo_ir_op_inv (ray_weights, cmd_queue, priv->op_inv_kernel);

    // Calculate pixel weights
    UfoBuffer *sino_tmp = ufo_buffer_dup (inputs[0]);
    ufo_ir_op_set (sino_tmp, 1.0f, cmd_queue, priv->op_set_kernel);
    UfoBuffer *pixel_weights = ufo_buffer_dup (output);
    ufo_ir_op_set (pixel_weights, 0.0f, cmd_queue, priv->op_set_kernel);
    ufo_ir_state_dependent_task_backward(sdprojector, &sino_tmp, pixel_weights, requisition);
    ufo_ir_op_inv (pixel_weights, cmd_queue, priv->op_inv_kernel);

    ufo_ir_projector_task_set_relaxation(projector, priv->relaxation_factor);
    ufo_ir_projector_task_set_correction_scale(projector, -1.0f);
    ufo_ir_op_set(output, 0.0f, cmd_queue, priv->op_set_kernel);

    // do SIRT
    guint iteration = 0;
    guint max_iterations = ufo_ir_method_task_get_iterations_number(UFO_IR_METHOD_TASK(task));
    while (iteration < max_iterations) {
        ufo_buffer_copy (inputs[0], sino_tmp);

        ufo_ir_state_dependent_task_forward(sdprojector, &output, sino_tmp, requisition);

        ufo_ir_op_mul (sino_tmp, ray_weights, sino_tmp, cmd_queue, priv->op_mul_kernel);
        ufo_ir_op_set (volume_tmp, 0, cmd_queue, priv->op_set_kernel);
        ufo_ir_state_dependent_task_backward(sdprojector, &sino_tmp, volume_tmp, requisition);

        ufo_ir_op_mul (volume_tmp, pixel_weights, volume_tmp, cmd_queue, priv->op_mul_kernel);
        ufo_ir_op_add (volume_tmp, output, output, cmd_queue, priv->op_add_kernel);

        iteration++;
    }

//    clFinish(cmd_queue);
//    g_timer_stop(timer);
//    gdouble _time = g_timer_elapsed (timer, NULL);
//    g_timer_destroy(timer);
//    g_print("%p %3.5f\n", cmd_queue, _time);

    g_object_unref(sino_tmp);
    g_object_unref(volume_tmp);
    g_object_unref(pixel_weights);
    g_object_unref(ray_weights);
    return TRUE;
}

static const gchar *ufo_ir_sirt_task_get_package_name(UfoTaskNode *self) {
    return g_strdup("ir");
}
// -----------------------------------------------------------------------------
