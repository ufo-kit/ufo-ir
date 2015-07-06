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

#include "ufo-ir-asdpocs-task.h"
#include "ufo-ir-basic-ops.h"
#include "ufo-ir-parallel-projector-task.h"

static void ufo_ir_asdpocs_task_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ufo_ir_asdpocs_task_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ufo_task_interface_init (UfoTaskIface *iface);
static void ufo_ir_asdpocs_task_setup (UfoTask *task, UfoResources *resources, GError **error);
static gboolean ufo_ir_asdpocs_task_process (UfoTask *task, UfoBuffer **inputs, UfoBuffer *output, UfoRequisition *requisition);
static void ufo_ir_asdpocs_task_finalize (GObject *object);
static const gchar *ufo_ir_asdpocs_task_get_package_name(UfoTaskNode *self);

struct _UfoIrAsdpocsTaskPrivate {
    // operation kernels
    gpointer op_set_kernel;
    gpointer op_inv_kernel;
    gpointer op_add_kernel;
    gpointer op_mul_kernel;

    // Method parameters
    gfloat beta;
    gfloat beta_red;
    guint  ng;
    gfloat alpha;
    gfloat alpha_red;
    gfloat r_max;
    gboolean positive_constraint;


};

G_DEFINE_TYPE_WITH_CODE (UfoIrAsdpocsTask, ufo_ir_asdpocs_task, UFO_IR_TYPE_METHOD_TASK,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_IR_ASDPOCS_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_ASDPOCS_TASK, UfoIrAsdpocsTaskPrivate))

enum {
    PROP_0,
    PROP_BETA,
    PROP_BETA_RED,
    PROP_NG,
    PROP_ALPHA,
    PROP_R_MAX,
    PROP_ALPHA_RED,
    PROP_POSITIVE_CONSTRAINT,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

// -----------------------------------------------------------------------------
// Init methods
// -----------------------------------------------------------------------------

static void
ufo_task_interface_init (UfoTaskIface *iface) {
    iface->process = ufo_ir_asdpocs_task_process;
    iface->setup = ufo_ir_asdpocs_task_setup;
}

static void
ufo_ir_asdpocs_task_class_init (UfoIrAsdpocsTaskClass *klass) {
    GObjectClass *oclass = G_OBJECT_CLASS (klass);

    oclass->set_property = ufo_ir_asdpocs_task_set_property;
    oclass->get_property = ufo_ir_asdpocs_task_get_property;
    oclass->finalize = ufo_ir_asdpocs_task_finalize;

    UfoTaskNodeClass * tnclass= UFO_TASK_NODE_CLASS(klass);
    tnclass->get_package_name = ufo_ir_asdpocs_task_get_package_name;

    properties[PROP_BETA] =
        g_param_spec_float("beta",
                           "Beta",
                           "Beta",
                           0.0f, G_MAXFLOAT, 1.0f,
                           G_PARAM_READWRITE);

    properties[PROP_BETA_RED] =
        g_param_spec_float("beta-red",
                           "Beta red",
                           "Beta red",
                            0.0f, G_MAXFLOAT, 0.995f,
                            G_PARAM_READWRITE);

    properties[PROP_NG] =
        g_param_spec_uint("ng",
                          "NG",
                          "NG",
                          0, 1000, 20,
                          G_PARAM_READWRITE);

    properties[PROP_ALPHA] =
        g_param_spec_float("alpha",
                           "Alpha",
                           "Alpha",
                           0.0f, G_MAXFLOAT, 0.2f,
                           G_PARAM_READWRITE);

    properties[PROP_ALPHA_RED] =
        g_param_spec_float("alpha-red",
                           "Alpha Red",
                           "Alpha Red",
                           0.0f, G_MAXFLOAT, 0.95f,
                           G_PARAM_READWRITE);

    properties[PROP_R_MAX] =
        g_param_spec_float("r-max",
                           "R-max",
                           "R-max",
                           0.0f, G_MAXFLOAT, 0.95f,
                           G_PARAM_READWRITE);

    properties[PROP_POSITIVE_CONSTRAINT] =
        g_param_spec_boolean("positive-constraint",
                             "Impose positive constraint",
                             "Impose positive constraint",
                             TRUE,
                             G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (oclass, i, properties[i]);

    g_type_class_add_private (oclass, sizeof(UfoIrAsdpocsTaskPrivate));
}

static void
ufo_ir_asdpocs_task_init(UfoIrAsdpocsTask *self) {
    UfoIrAsdpocsTaskPrivate *priv = NULL;
    self->priv = priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE(self);

    //priv->df_minimizer = NULL;
    //priv->tv_stdesc = NULL;
    priv->beta = 1.0f;
    priv->beta_red = 0.995f;
    priv->ng = 20;
    priv->alpha = 0.2f;
    priv->alpha_red = 0.95f;
    priv->r_max = 0.95f;
    priv->positive_constraint = TRUE;
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Finalization
// -----------------------------------------------------------------------------

static void
ufo_ir_asdpocs_task_finalize (GObject *object) {
    G_OBJECT_CLASS (ufo_ir_asdpocs_task_parent_class)->finalize (object);
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Getters and setters
// -----------------------------------------------------------------------------

gfloat ufo_ir_asdpocs_task_get_beta(UfoIrAsdpocsTask *self) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    return priv->beta;
}

void ufo_ir_asdpocs_task_set_beta(UfoIrAsdpocsTask *self, gfloat value) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    priv->beta = value;
}

gfloat ufo_ir_asdpocs_task_get_beta_red(UfoIrAsdpocsTask *self) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    return priv->beta_red;
}

void ufo_ir_asdpocs_task_set_beta_red(UfoIrAsdpocsTask *self, gfloat value) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    priv->beta_red = value;
}

guint ufo_ir_asdpocs_task_get_ng(UfoIrAsdpocsTask *self) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    return priv->ng;
}

void ufo_ir_asdpocs_task_set_ng(UfoIrAsdpocsTask *self, guint value) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    priv->ng = value;
}

gfloat ufo_ir_asdpocs_task_get_alpha(UfoIrAsdpocsTask *self) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    return priv->alpha;
}

void ufo_ir_asdpocs_task_set_alpha(UfoIrAsdpocsTask *self, gfloat value) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    priv->alpha = value;
}

gfloat ufo_ir_asdpocs_task_get_alpha_red(UfoIrAsdpocsTask *self) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    return priv->alpha_red;
}

void ufo_ir_asdpocs_task_set_alpha_red(UfoIrAsdpocsTask *self, gfloat value) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    priv->alpha_red = value;
}

gfloat ufo_ir_asdpocs_task_get_r_max(UfoIrAsdpocsTask *self) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    return priv->r_max;
}

void ufo_ir_asdpocs_task_set_r_max(UfoIrAsdpocsTask *self, gfloat value) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    priv->r_max = value;
}

gboolean ufo_ir_asdpocs_task_get_positive_constraint(UfoIrAsdpocsTask *self) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    return priv->positive_constraint;
}

void ufo_ir_asdpocs_task_set_positive_constraint(UfoIrAsdpocsTask *self, gboolean value) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    priv->positive_constraint = value;
}

static void
ufo_ir_asdpocs_task_set_property (GObject *object,
                                  guint property_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
    UfoIrAsdpocsTask *self = UFO_IR_ASDPOCS_TASK (object);

    switch (property_id) {
        case PROP_BETA:
            ufo_ir_asdpocs_task_set_beta(self, g_value_get_float(value));
        break;
        case PROP_BETA_RED:
            ufo_ir_asdpocs_task_set_beta_red(self, g_value_get_float(value));
            break;
        case PROP_NG:
            ufo_ir_asdpocs_task_set_ng(self, g_value_get_uint(value));
            break;
        case PROP_ALPHA:
            ufo_ir_asdpocs_task_set_alpha(self, g_value_get_float (value));
            break;
        case PROP_ALPHA_RED:
            ufo_ir_asdpocs_task_set_alpha_red(self, g_value_get_float (value));
            break;
        case PROP_R_MAX:
            ufo_ir_asdpocs_task_set_r_max(self, g_value_get_float (value));
            break;
        case PROP_POSITIVE_CONSTRAINT:
            ufo_ir_asdpocs_task_set_positive_constraint(self, g_value_get_boolean(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_asdpocs_task_get_property (GObject *object,
                                  guint property_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
    UfoIrAsdpocsTask *self = UFO_IR_ASDPOCS_TASK (object);

    switch (property_id) {
        case PROP_BETA:
            g_value_set_float (value, ufo_ir_asdpocs_task_get_beta(self));
            break;
        case PROP_BETA_RED:
            g_value_set_float (value, ufo_ir_asdpocs_task_get_beta_red(self));
            break;
        case PROP_NG:
            g_value_set_uint (value, ufo_ir_asdpocs_task_get_ng(self));
            break;
        case PROP_ALPHA:
            g_value_set_float (value, ufo_ir_asdpocs_task_get_alpha(self));
            break;
        case PROP_ALPHA_RED:
            g_value_set_float (value, ufo_ir_asdpocs_task_get_alpha_red(self));
            break;
        case PROP_R_MAX:
            g_value_set_float (value, ufo_ir_asdpocs_task_get_r_max(self));
            break;
        case PROP_POSITIVE_CONSTRAINT:
            g_value_set_boolean (value, ufo_ir_asdpocs_task_get_positive_constraint(self));
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
ufo_ir_asdpocs_task_new (void) {
    return UFO_NODE (g_object_new (UFO_IR_TYPE_ASDPOCS_TASK, NULL));
}

static void
ufo_ir_asdpocs_task_setup (UfoTask      *task,
                        UfoResources *resources,
                        GError       **error)
{
    ufo_task_node_set_proc_node(UFO_TASK_NODE(ufo_ir_method_task_get_projector(UFO_IR_METHOD_TASK(task))), ufo_task_node_get_proc_node(UFO_TASK_NODE(task)));

    ufo_task_setup(UFO_TASK(ufo_ir_method_task_get_projector(UFO_IR_METHOD_TASK(task))), resources, error);
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (task);
    priv->op_set_kernel = ufo_ir_op_set_generate_kernel(resources);
    priv->op_inv_kernel = ufo_ir_op_inv_generate_kernel(resources);
    priv->op_add_kernel = ufo_ir_op_add_generate_kernel(resources);
    priv->op_mul_kernel = ufo_ir_op_mul_generate_kernel(resources);
}

static gboolean
ufo_ir_asdpocs_task_process (UfoTask *task,
                          UfoBuffer **inputs,
                          UfoBuffer *output,
                          UfoRequisition *requisition) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (task);
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

static const gchar *ufo_ir_asdpocs_task_get_package_name(UfoTaskNode *self) {
    return g_strdup("ir");
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

    UfoIrProjectionsSubset *subsets_tmp =
        g_malloc (sizeof (UfoIrProjectionsSubset) * n_angles);

    guint subset_index = 0;
    UfoIrProjectionDirection direction;
    direction = fabs(sin_values[0]) <= fabs(cos_values[0]);
    subsets_tmp[subset_index].direction = direction;
    subsets_tmp[subset_index].offset = 0;
    subsets_tmp[subset_index].n = 1;

    for (guint i = 1; i < n_angles; ++i) {
        direction = fabs(sin_values[i]) <= fabs(cos_values[i]);
        if (direction != subsets_tmp[subset_index].direction) {
            subset_index++;
            subsets_tmp[subset_index].direction = direction;
            subsets_tmp[subset_index].offset = i;
            subsets_tmp[subset_index].n = 1;
        } else {
            subsets_tmp[subset_index].n++;
        }
    }

    *n_subsets = subset_index + 1;
    UfoIrProjectionsSubset *subsets =
        g_memdup (subsets_tmp, sizeof (UfoIrProjectionsSubset) * (*n_subsets));

    g_free (subsets_tmp);

    return subsets;
}
// -----------------------------------------------------------------------------
