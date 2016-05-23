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
#include <math.h>

#include "ufo-ir-asdpocs-task.h"
#include "core/ufo-ir-basic-ops.h"
#include "ufo-ir-parallel-projector-task.h"

static void ufo_ir_asdpocs_task_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ufo_ir_asdpocs_task_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ufo_task_interface_init (UfoTaskIface *iface);
static void ufo_ir_asdpocs_task_setup (UfoTask *task, UfoResources *resources, GError **error);
static gboolean ufo_ir_asdpocs_task_process (UfoTask *task, UfoBuffer **inputs, UfoBuffer *output, UfoRequisition *requisition);
static void ufo_ir_asdpocs_task_dispose (GObject *object);
static UfoIrProjectionsSubset *generate_subsets (UfoIrParallelProjectorTask *projector, guint *n_subsets);
static void ufo_math_tvstd_method_process_real (UfoIrAsdpocsTask *self, UfoBuffer *input, UfoBuffer *output, gfloat relaxation, cl_command_queue cmd_queue);

struct _UfoIrAsdpocsTaskPrivate {
    // operation kernels
    gpointer op_set_kernel;
    gpointer op_inv_kernel;
    gpointer op_add_kernel;
    gpointer op_mul_kernel;
    gpointer op_ded_kernel;
    gpointer op_pos_kernel;
    gpointer op_dd2_kernel;

    // Method parameters
    gfloat beta;
    gfloat beta_red;
    guint  ng;
    gfloat alpha;
    gfloat alpha_red;
    gfloat r_max;
    gboolean positive_constraint;

    // tvstd related
    cl_kernel tvstd;
    UfoBuffer *grad_temp_buffer;

    // df_minimizer
    UfoTask *df_minimizer;
};

G_DEFINE_TYPE_WITH_CODE (UfoIrAsdpocsTask, ufo_ir_asdpocs_task, UFO_IR_TYPE_METHOD_TASK,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_IR_ASDPOCS_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_ASDPOCS_TASK, UfoIrAsdpocsTaskPrivate))

enum {
    PROP_0,
    PROP_BETA,
    PROP_BETA_RED,
    PROP_ALPHA,
    PROP_ALPHA_RED,
    PROP_R_MAX,
    PROP_NG,
    PROP_POSITIVE_CONSTRAINT,
    PROP_DF_MINIMIZER,
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
    oclass->dispose = ufo_ir_asdpocs_task_dispose;

    properties[PROP_BETA] =
        g_param_spec_float("beta",
                           "Beta",
                           "Beta",
                           0.0f, G_MAXFLOAT, 1.0f,
                           G_PARAM_READWRITE);

    properties[PROP_BETA_RED] =
        g_param_spec_float("beta_red",
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
        g_param_spec_float("alpha_red",
                           "Alpha Red",
                           "Alpha Red",
                           0.0f, G_MAXFLOAT, 0.95f,
                           G_PARAM_READWRITE);

    properties[PROP_R_MAX] =
        g_param_spec_float("r_max",
                           "R-max",
                           "R-max",
                           0.0f, G_MAXFLOAT, 0.95f,
                           G_PARAM_READWRITE);

    properties[PROP_POSITIVE_CONSTRAINT] =
        g_param_spec_boolean("positive_constraint",
                             "Impose positive constraint",
                             "Impose positive constraint",
                             TRUE,
                             G_PARAM_READWRITE);

    properties[PROP_DF_MINIMIZER] =
            g_param_spec_object("df_minimizer",
                                "Current minimizer",
                                "Current minimizer",
                                UFO_TYPE_TASK,
                                G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (oclass, i, properties[i]);

    g_type_class_add_private (oclass, sizeof(UfoIrAsdpocsTaskPrivate));
}

static void
ufo_ir_asdpocs_task_init(UfoIrAsdpocsTask *self) {
    UfoIrAsdpocsTaskPrivate *priv = NULL;
    self->priv = priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE(self);

    priv->df_minimizer = NULL;
    priv->beta = 1.0f;
    priv->beta_red = 0.995f;
    priv->ng = 20;
    priv->alpha = 0.2f;
    priv->alpha_red = 0.95f;
    priv->r_max = 0.95f;
    priv->positive_constraint = TRUE;
    priv->grad_temp_buffer = NULL;
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Finalization
// -----------------------------------------------------------------------------

static void
ufo_ir_asdpocs_task_dispose (GObject *object)
{
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (object);
    g_object_unref (priv->df_minimizer);
    priv->df_minimizer = NULL;
    G_OBJECT_CLASS (ufo_ir_asdpocs_task_parent_class)->dispose (object);
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

UfoTask *ufo_ir_asdpocs_task_get_df_minimizer(UfoIrAsdpocsTask *self) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    return priv->df_minimizer;
}

void ufo_ir_asdpocs_task_set_df_minimizer(UfoIrAsdpocsTask *self, UfoTask *value) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (self);
    if(priv->df_minimizer != NULL) {
        g_object_unref(priv->df_minimizer);
    }

    priv->df_minimizer = g_object_ref(value);
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
        case PROP_DF_MINIMIZER:
            ufo_ir_asdpocs_task_set_df_minimizer(self, g_value_get_object(value));
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
        case PROP_DF_MINIMIZER:
            g_value_set_object(value, ufo_ir_asdpocs_task_get_df_minimizer(self));
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
ufo_ir_asdpocs_task_setup (UfoTask *task,
                           UfoResources *resources,
                           GError **error)
{
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE (task);

    // Init projector
    UfoIrProjectorTask *projector = ufo_ir_method_task_get_projector(UFO_IR_METHOD_TASK(task));
    ufo_task_node_set_proc_node(UFO_TASK_NODE(projector), ufo_task_node_get_proc_node(UFO_TASK_NODE(task)));
    ufo_task_setup(UFO_TASK(ufo_ir_method_task_get_projector(UFO_IR_METHOD_TASK(task))), resources, error);

    // Init df_minimizer
    if (priv->df_minimizer == NULL) {
        g_set_error (error, UFO_TASK_ERROR, UFO_TASK_ERROR_SETUP, "df_minimizer is not defined");
        return;
    }

    ufo_task_node_set_proc_node(UFO_TASK_NODE(priv->df_minimizer), ufo_task_node_get_proc_node(UFO_TASK_NODE(task)));
    ufo_ir_method_task_set_projector(UFO_IR_METHOD_TASK(priv->df_minimizer), projector);
    ufo_task_setup(priv->df_minimizer, resources, error);

    // Load op kernels
    priv->op_set_kernel = ufo_ir_op_set_generate_kernel(resources);
    priv->op_inv_kernel = ufo_ir_op_inv_generate_kernel(resources);
    priv->op_add_kernel = ufo_ir_op_add_generate_kernel(resources);
    priv->op_mul_kernel = ufo_ir_op_mul_generate_kernel(resources);
    priv->op_ded_kernel = ufo_ir_op_deduction_generate_kernel(resources);
    priv->op_dd2_kernel = ufo_ir_op_deduction2_generate_kernel(resources);
    priv->op_pos_kernel = ufo_ir_op_positive_constraint_generate_kernel(resources);

    // Load tvstd kernel
    priv->tvstd = ufo_resources_get_kernel (resources, "ufo-math-tvstd-method.cl", "l1_grad", error);
}

static gboolean
ufo_ir_asdpocs_task_process (UfoTask *task,
                             UfoBuffer **inputs,
                             UfoBuffer *output,
                             UfoRequisition *requisition) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE(task);

    // Check and setup temp buffer
    if(priv->grad_temp_buffer) {
        UfoRequisition grad_req;
        ufo_buffer_get_requisition(priv->grad_temp_buffer, &grad_req);
        if( grad_req.dims[0] == requisition->dims[0] && grad_req.dims[1] == requisition->dims[1] )
        {
            g_object_unref(priv->grad_temp_buffer);
            priv->grad_temp_buffer = ufo_buffer_dup(output);
        }
    }
    else {
        priv->grad_temp_buffer = ufo_buffer_dup(output);
    }


    UfoIrParallelProjectorTask *projector = UFO_IR_PARALLEL_PROJECTOR_TASK(ufo_ir_method_task_get_projector(UFO_IR_METHOD_TASK(task)));
    ufo_ir_method_task_set_projector(UFO_IR_METHOD_TASK(priv->df_minimizer), UFO_IR_PROJECTOR_TASK(projector));
    UfoGpuNode *node = UFO_GPU_NODE (ufo_task_node_get_proc_node (UFO_TASK_NODE(task)));
    cl_command_queue cmd_queue = (cl_command_queue)ufo_gpu_node_get_cmd_queue (node);

    // parameters
    gfloat dp = 1.0f, dd = 1.0f, dg = 1.0f, dtgv = 1.0f;

    UfoBuffer *x = ufo_buffer_dup (output);
    ufo_ir_op_set (x, 0, cmd_queue, priv->op_set_kernel);

    UfoBuffer *x_prev = ufo_buffer_dup (output);
    ufo_ir_op_set (x_prev, 0, cmd_queue, priv->op_set_kernel);

    UfoBuffer *x_residual = ufo_buffer_dup (output);
    UfoBuffer *b_residual = ufo_buffer_dup (inputs[0]);

    guint n_subsets = 0;
    UfoIrProjectionsSubset *subsets = generate_subsets (projector, &n_subsets);

    gfloat beta = priv->beta;
    guint iteration = 0;
    guint max_iterations = ufo_ir_method_task_get_iterations_number(UFO_IR_METHOD_TASK(task));
    while (iteration < max_iterations)
    {
        // run method to minimize data fidelity term
        g_object_set (priv->df_minimizer, "relaxation_factor", beta, NULL);
        ufo_task_process(priv->df_minimizer, inputs, x, NULL);

        // impose positive constraint: if x_i < 0 then x_i = 0
        if (priv->positive_constraint) {
            ufo_ir_op_positive_constraint(x, x, cmd_queue, priv->op_pos_kernel);
        }

        // save result as an result
        ufo_buffer_copy (x, output);

        // Find residual between the simulated and real measurements
        ufo_buffer_copy (inputs[0], b_residual);
        ufo_ir_projector_task_set_correction_scale(UFO_IR_PROJECTOR_TASK(projector), -1.0f);
        for (guint i = 0 ; i < n_subsets; ++i) {
            ufo_ir_parallel_projector_subset_fp(projector, x, b_residual, &subsets[i]);
        }

        // compute L1-norm of the residual of measurements
        dd = ufo_ir_op_l1_norm (b_residual, cmd_queue);

        // compute L1-norm of the residual of reconstructions
        ufo_ir_op_deduction (x, x_prev, x_residual, cmd_queue, priv->op_ded_kernel);
        dp = ufo_ir_op_l1_norm (x_residual, cmd_queue);

        // compute relaxation factor for minimizing regularization term
        if (iteration == 0) {
            dtgv = priv->alpha * dp;
        }

        // save current solution
        ufo_buffer_copy (x, x_prev);

        ufo_math_tvstd_method_process_real(UFO_IR_ASDPOCS_TASK(task), x, x, dtgv, cmd_queue);

        // compute new regularization coefficient
        const gfloat epsilon = 0.001f;
        ufo_ir_op_deduction (x, x_prev, x_residual, cmd_queue, priv->op_ded_kernel);
        dg = ufo_ir_op_l1_norm (x_residual, cmd_queue);
        beta *= priv->beta_red;

        // compute relaxation factor for minimizing regularization term
        if (dg > priv->r_max * dp && dd > epsilon) {
            dtgv *= priv->alpha_red;
        }

        iteration++;
    }

    return TRUE;
}


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

static void
ufo_math_tvstd_method_process_real (UfoIrAsdpocsTask *self,
                                    UfoBuffer *input,
                                    UfoBuffer *output,
                                    gfloat relaxation,
                                    cl_command_queue cmd_queue) {
    UfoIrAsdpocsTaskPrivate *priv = UFO_IR_ASDPOCS_TASK_GET_PRIVATE(self);

    ufo_ir_op_set (priv->grad_temp_buffer, 0, cmd_queue, priv->op_set_kernel);

    UfoRequisition input_req;
    ufo_buffer_get_requisition (input, &input_req);

    cl_mem d_input = ufo_buffer_get_device_image (input, cmd_queue);
    cl_mem d_grad = ufo_buffer_get_device_image (priv->grad_temp_buffer, cmd_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (priv->tvstd, 0, sizeof(cl_mem), &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (priv->tvstd, 1, sizeof(cl_mem), &d_grad));

    gfloat factor = 0.0f, l1 = 0.0f;
    guint iteration = 0;

    while (iteration < 20) {
        UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (cmd_queue, priv->tvstd, input_req.n_dims, NULL, input_req.dims, NULL, 0, NULL, NULL));
        l1 = ufo_ir_op_l1_norm (priv->grad_temp_buffer, cmd_queue);
        factor = relaxation / l1;
        ufo_ir_op_deduction2 (input, priv->grad_temp_buffer, factor, output, cmd_queue, priv->op_dd2_kernel);
        iteration++;
    }
}

// -----------------------------------------------------------------------------
