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

#include "ufo-ir-sart-task.h"
#include "core/ufo-ir-basic-ops.h"
#include "ufo-ir-parallel-projector-task.h"
#include <math.h>

static void ufo_ir_sart_task_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ufo_ir_sart_task_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ufo_task_interface_init (UfoTaskIface *iface);
static void ufo_ir_sart_task_setup (UfoTask *task, UfoResources *resources, GError **error);
static gboolean ufo_ir_sart_task_process (UfoTask *task, UfoBuffer **inputs, UfoBuffer *output, UfoRequisition *requisition);
static UfoIrProjectionsSubset *generate_subsets (UfoIrParallelProjectorTask *projector, guint *n_subsets);
static const gchar *ufo_ir_sart_task_get_package_name(UfoTaskNode *self);

struct _UfoIrSartTaskPrivate {
    gfloat relaxation_factor;
    gpointer op_set_kernel;
    gpointer op_inv_kernel;
    gpointer op_add_kernel;
    gpointer op_mul_kernel;
    gpointer op_mul_rows_kernel;
};

G_DEFINE_TYPE_WITH_CODE (UfoIrSartTask, ufo_ir_sart_task, UFO_IR_TYPE_METHOD_TASK,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_IR_SART_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_SART_TASK, UfoIrSartTaskPrivate))

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
    iface->process = ufo_ir_sart_task_process;
    iface->setup = ufo_ir_sart_task_setup;
}

static void
ufo_ir_sart_task_class_init (UfoIrSartTaskClass *klass) {
    GObjectClass *oclass = G_OBJECT_CLASS (klass);

    oclass->set_property = ufo_ir_sart_task_set_property;
    oclass->get_property = ufo_ir_sart_task_get_property;

    UfoTaskNodeClass * tnclass= UFO_TASK_NODE_CLASS(klass);
    tnclass->get_package_name = ufo_ir_sart_task_get_package_name;

    properties[PROP_RELAXATION_FACTOR] =
            g_param_spec_float("relaxation_factor",
                               "relaxation_factor",
                               "Relaxation factor",
                               0.0f, 1.0f, 0.5f,
                               G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (oclass, i, properties[i]);

    g_type_class_add_private (oclass, sizeof(UfoIrSartTaskPrivate));
}

static void
ufo_ir_sart_task_init(UfoIrSartTask *self) {
    self->priv = UFO_IR_SART_TASK_GET_PRIVATE(self);
    self->priv->relaxation_factor = 0.25;
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Getters and setters
// -----------------------------------------------------------------------------
static void
ufo_ir_sart_task_set_property (GObject *object,
                               guint property_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
    UfoIrSartTask *self = UFO_IR_SART_TASK (object);

    switch (property_id) {
        case PROP_RELAXATION_FACTOR:
            ufo_ir_sart_task_set_relaxation_factor(self, g_value_get_float(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_sart_task_get_property (GObject *object,
                               guint property_id,
                               GValue *value,
                               GParamSpec *pspec)
{
    UfoIrSartTask *self = UFO_IR_SART_TASK (object);

    switch (property_id) {
        case PROP_RELAXATION_FACTOR:
            g_value_set_float(value, ufo_ir_sart_task_get_relaxation_factor(self));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

gfloat
ufo_ir_sart_task_get_relaxation_factor(UfoIrSartTask *self) {
    UfoIrSartTaskPrivate *priv = UFO_IR_SART_TASK_GET_PRIVATE (self);
    return priv->relaxation_factor;
}

void
ufo_ir_sart_task_set_relaxation_factor(UfoIrSartTask *self, gfloat value) {
    UfoIrSartTaskPrivate *priv = UFO_IR_SART_TASK_GET_PRIVATE (self);
    priv->relaxation_factor = value;
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Task interface realization
// -----------------------------------------------------------------------------
UfoNode *
ufo_ir_sart_task_new (void) {
    return UFO_NODE (g_object_new (UFO_IR_TYPE_SART_TASK, NULL));
}

static void
ufo_ir_sart_task_setup (UfoTask      *task,
                        UfoResources *resources,
                        GError       **error)
{
    if(!UFO_IR_IS_PARALLEL_PROJECTOR_TASK(ufo_ir_method_task_get_projector(UFO_IR_METHOD_TASK(task)))) {
        g_error("Wrong projector type");
    }

    ufo_task_node_set_proc_node(UFO_TASK_NODE(ufo_ir_method_task_get_projector(UFO_IR_METHOD_TASK(task))), ufo_task_node_get_proc_node(UFO_TASK_NODE(task)));

    ufo_task_setup(UFO_TASK(ufo_ir_method_task_get_projector(UFO_IR_METHOD_TASK(task))), resources, error);
    UfoIrSartTaskPrivate *priv = UFO_IR_SART_TASK_GET_PRIVATE (task);
    priv->op_set_kernel = ufo_ir_op_set_generate_kernel(resources);
    priv->op_inv_kernel = ufo_ir_op_inv_generate_kernel(resources);
    priv->op_add_kernel = ufo_ir_op_add_generate_kernel(resources);
    priv->op_mul_kernel = ufo_ir_op_mul_generate_kernel(resources);
    priv->op_mul_rows_kernel = ufo_ir_op_mul_rows_generate_kernel(resources);
}

static gboolean
ufo_ir_sart_task_process (UfoTask *task,
                          UfoBuffer **inputs,
                          UfoBuffer *output,
                          UfoRequisition *requisition) {

    UfoIrSartTaskPrivate *priv = UFO_IR_SART_TASK_GET_PRIVATE (task);
    UfoIrParallelProjectorTask *projector = UFO_IR_PARALLEL_PROJECTOR_TASK(ufo_ir_method_task_get_projector(UFO_IR_METHOD_TASK(task)));
    UfoGpuNode *node = UFO_GPU_NODE (ufo_task_node_get_proc_node (UFO_TASK_NODE(task)));
    cl_command_queue cmd_queue = (cl_command_queue)ufo_gpu_node_get_cmd_queue (node);

//    GTimer *timer = g_timer_new ();
//    g_timer_reset(timer);
//    clFinish(cmd_queue);
//    g_timer_start(timer);

    UfoBuffer *sino_tmp = ufo_buffer_dup (inputs[0]);
    UfoBuffer *volume_tmp = ufo_buffer_dup (output);

    UfoBuffer *ray_weights = ufo_buffer_dup (inputs[0]);

    guint n_subsets = 0;
    UfoIrProjectionsSubset *subsets = generate_subsets (projector, &n_subsets);

    // calculate the weighting coefficients
    ufo_ir_op_set (volume_tmp,  1.0f, cmd_queue, priv->op_set_kernel);
    ufo_ir_op_set (ray_weights, 0.0f, cmd_queue, priv->op_set_kernel);
    ufo_ir_projector_task_set_correction_scale(UFO_IR_PROJECTOR_TASK(projector), 1.0f);
    for (guint i = 0 ; i < n_subsets; ++i) {
        ufo_ir_parallel_projector_subset_fp(projector, volume_tmp, ray_weights, &subsets[i]);
    }

    ufo_ir_op_inv (ray_weights, cmd_queue, priv->op_inv_kernel);

    // do SART
    guint max_iterations = ufo_ir_method_task_get_iterations_number(UFO_IR_METHOD_TASK(task));
    guint iteration = 0;
    ufo_ir_projector_task_set_correction_scale(UFO_IR_PROJECTOR_TASK(projector), -1.0f);
    ufo_ir_projector_task_set_relaxation(UFO_IR_PROJECTOR_TASK(projector), priv->relaxation_factor);
    ufo_ir_op_set(output, 0.0f, cmd_queue, priv->op_set_kernel);
    while (iteration < max_iterations) {
        ufo_buffer_copy (inputs[0], sino_tmp);

        for (guint i = 0 ; i < n_subsets; i++) {
            ufo_ir_parallel_projector_subset_fp(projector, output, sino_tmp, &subsets[i]);

            ufo_ir_op_mul_rows (sino_tmp, ray_weights, sino_tmp, subsets[i].offset, subsets[i].n, cmd_queue, priv->op_mul_rows_kernel);

            ufo_ir_parallel_projector_subset_bp (projector, output, sino_tmp, &subsets[i]);
        }

        iteration++;
    }

    g_object_unref(sino_tmp);
    g_object_unref(volume_tmp);
    g_object_unref(ray_weights);
    g_free(subsets);


//    clFinish(cmd_queue);
//    g_timer_stop(timer);
//    gdouble _time = g_timer_elapsed (timer, NULL);
//    g_timer_destroy(timer);
//    g_print("%p %3.5f\n", cmd_queue, _time);
    return TRUE;
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------------
static UfoIrProjectionsSubset *
generate_subsets (UfoIrParallelProjectorTask *projector, guint *n_subsets)
{
    const gfloat *sin_values = ufo_ir_parallel_projector_get_host_sin_vals(projector);
    const gfloat *cos_values = ufo_ir_parallel_projector_get_host_cos_vals(projector);
    guint n_angles = ufo_ir_parallel_projector_get_angles_num(projector);
    *n_subsets = n_angles;

    UfoIrProjectionsSubset *subsets =
        (UfoIrProjectionsSubset *)g_malloc (sizeof (UfoIrProjectionsSubset) * n_angles);

    for (guint i = 0; i < n_angles; ++i) {
        subsets[i].direction = fabs(sin_values[i]) <= fabs(cos_values[i]); // vertical == 1
        subsets[i].offset = i;
        subsets[i].n = 1;
    }

    return subsets;
}

static const gchar *
ufo_ir_sart_task_get_package_name (UfoTaskNode *self)
{
    return "ir";
}

// -----------------------------------------------------------------------------
