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

#include "ufo-ir-parallel-projector-task.h"
#include <math.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#define OPS_FILENAME "ufo-basic-ops.cl"

struct _UfoIrParallelProjectorTaskPrivate {
    cl_context context;

    // Precompiled sin/cos values
    cl_mem scan_sin_lut;
    cl_mem scan_cos_lut;
    gfloat *scan_host_sin_lut;
    gfloat *scan_host_cos_lut;

    gchar *model_name; // Projection model name
    gpointer fp_kernel[2]; // Forward projections kernels
    gpointer bp_kernel; // Backprojection kernel
    gpointer set_kernel;

    guint detectors_num;
    guint angles_num;
    UfoIrProjectionsSubset *full_subsets_list;
    guint full_subsets_cnt;
};

static void ufo_task_interface_init (UfoTaskIface *iface);
static void ufo_ir_parallel_projector_task_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ufo_ir_parallel_projector_task_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ufo_ir_parallel_projector_task_setup (UfoTask *self, UfoResources *resources, GError **error);
static void ufo_ir_parallel_projector_task_get_requisition (UfoTask *self, UfoBuffer **inputs, UfoRequisition *requisition);
static UfoTaskMode ufo_ir_parallel_projector_task_get_mode (UfoTask *task);
// Private methods
static cl_mem create_lut_buffer (guint angles_num, gfloat angles_step, const cl_context *context, gfloat **host_mem, double (*func)(double));
static UfoIrProjectionsSubset *generate_full_subsets_list (UfoIrParallelProjectorTaskPrivate *priv);
static void ufo_ir_parallel_projector_subset_bp_real(UfoIrParallelProjectorTask *self, UfoBuffer *volume, UfoBuffer *sinogram, UfoIrProjectionsSubset *subset, UfoRequisition *requisitions, cl_command_queue cmd_queue);
static void ufo_ir_parallel_projector_subset_fp_real(UfoIrParallelProjectorTask *self, UfoBuffer *volume, UfoBuffer *sinogram, UfoIrProjectionsSubset *subset, UfoRequisition *requisitions, cl_command_queue cmd_queue);
// State dependent methods
gboolean ufo_ir_parallel_projector_task_forward(UfoIrStateDependentTask *self, UfoBuffer **inputs, UfoBuffer *output, UfoRequisition *requisition);
gboolean ufo_ir_parallel_projector_task_backward(UfoIrStateDependentTask *self, UfoBuffer **inputs, UfoBuffer *output, UfoRequisition *requisition);

G_DEFINE_TYPE_WITH_CODE (UfoIrParallelProjectorTask, ufo_ir_parallel_projector_task, UFO_IR_TYPE_PROJECTOR_TASK,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_PARALLEL_PROJECTOR_TASK, UfoIrParallelProjectorTaskPrivate))

enum {
    PROP_0 = 200,
    PROP_MODEL,
    PROP_ANGLES_NUM,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoNode *
ufo_ir_parallel_projector_task_new (void) {
    return UFO_NODE (g_object_new (UFO_IR_TYPE_PARALLEL_PROJECTOR_TASK, NULL));
}

static void
ufo_ir_parallel_projector_task_finalize (GObject *object)
{
    UfoIrParallelProjectorTaskPrivate *priv;

    priv = UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE (object);

    if (priv->scan_sin_lut) {
        UFO_RESOURCES_CHECK_CLERR (clReleaseMemObject (priv->scan_sin_lut));
        priv->scan_sin_lut = NULL;
    }

    if (priv->scan_cos_lut) {
        UFO_RESOURCES_CHECK_CLERR (clReleaseMemObject (priv->scan_cos_lut));
        priv->scan_cos_lut = NULL;
    }

    G_OBJECT_CLASS (ufo_ir_parallel_projector_task_parent_class)->finalize (object);
}

static void
ufo_task_interface_init (UfoTaskIface *iface) {
    iface->setup = ufo_ir_parallel_projector_task_setup;
    iface->get_mode = ufo_ir_parallel_projector_task_get_mode;
    iface->get_requisition = ufo_ir_parallel_projector_task_get_requisition;
}

static void
ufo_ir_parallel_projector_task_class_init (UfoIrParallelProjectorTaskClass *klass) {
    GObjectClass *oclass = G_OBJECT_CLASS (klass);

    oclass->set_property = ufo_ir_parallel_projector_task_set_property;
    oclass->get_property = ufo_ir_parallel_projector_task_get_property;
    oclass->finalize = ufo_ir_parallel_projector_task_finalize;

    UfoIrStateDependentTaskClass *sdclass = UFO_IR_STATE_DEPENDENT_TASK_CLASS(klass);
    sdclass->forward = ufo_ir_parallel_projector_task_forward;
    sdclass->backward = ufo_ir_parallel_projector_task_backward;

    properties[PROP_MODEL] =
        g_param_spec_string ("model",
                             "The name of the projection model.",
                             "The name of the projection model.",
                             "joseph",
                              G_PARAM_READWRITE);

    properties[PROP_ANGLES_NUM] =
        g_param_spec_uint ("angles_num",
                           "Amount of angles for forward projection",
                           "Amount of angles for forward projection",
                           1, G_MAXUINT, 1,
                           G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (oclass, i, properties[i]);

    g_type_class_add_private (oclass, sizeof(UfoIrParallelProjectorTaskPrivate));
}

static void
ufo_ir_parallel_projector_task_init(UfoIrParallelProjectorTask *self) {
    self->priv = UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE(self);
    self->priv->model_name = g_strdup("joseph");
    self->priv->angles_num = 1;
}

// -----------------------------------------------------------------------------
// Public methods
// -----------------------------------------------------------------------------

const gfloat *ufo_ir_parallel_projector_get_host_sin_vals(UfoIrParallelProjectorTask *self) {
    UfoIrParallelProjectorTaskPrivate *priv = UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE(self);
    return priv->scan_host_sin_lut;
}

const gfloat *ufo_ir_parallel_projector_get_host_cos_vals(UfoIrParallelProjectorTask *self) {
    UfoIrParallelProjectorTaskPrivate *priv = UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE(self);
    return priv->scan_host_cos_lut;
}

void ufo_ir_parallel_projector_subset_fp(UfoIrParallelProjectorTask *self,
                                             UfoBuffer *volume,
                                             UfoBuffer *sinogram,
                                             UfoIrProjectionsSubset *subset) {
    UfoGpuNode *node = UFO_GPU_NODE (ufo_task_node_get_proc_node (UFO_TASK_NODE (self)));
    cl_command_queue cmd_queue = ufo_gpu_node_get_cmd_queue (node);

    UfoRequisition req;
    ufo_buffer_get_requisition(sinogram, &req);

    ufo_ir_parallel_projector_subset_fp_real(self, volume, sinogram, subset, &req, cmd_queue);

}

void
ufo_ir_parallel_projector_subset_bp(UfoIrParallelProjectorTask *self,
                                    UfoBuffer *volume,
                                    UfoBuffer *sinogram,
                                    UfoIrProjectionsSubset *subset) {


    UfoGpuNode *node = UFO_GPU_NODE (ufo_task_node_get_proc_node (UFO_TASK_NODE (self)));
    cl_command_queue cmd_queue = ufo_gpu_node_get_cmd_queue (node);

    UfoRequisition req;
    ufo_buffer_get_requisition(volume, &req);

    ufo_ir_parallel_projector_subset_bp_real(self, volume, sinogram, subset, &req, cmd_queue);
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Getters and setters
// -----------------------------------------------------------------------------

/**
 * ufo_ir_parallel_projector_task_set_property:
 * @object object
 * @property_id property_id
 * @value value
 * @pspec pspec
 *
 * Usual GObject setter
 */
static void
ufo_ir_parallel_projector_task_set_property (GObject *object,
                                             guint property_id,
                                             const GValue *value,
                                             GParamSpec *pspec) {
    UfoIrParallelProjectorTask *self = UFO_IR_PARALLEL_PROJECTOR_TASK(object);

    switch (property_id) {
        case PROP_MODEL:
            ufo_ir_parallel_projector_set_model (self, g_value_get_string (value));
            break;
        case PROP_ANGLES_NUM:
            ufo_ir_parallel_projector_set_angles_num(self, g_value_get_uint(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

/**
 * ufo_ir_parallel_projector_task_get_property:
 * @object object
 * @property_id property_id
 * @value value
 * @pspec pspec
 *
 * Usual GObject getter
 */
static void
ufo_ir_parallel_projector_task_get_property (GObject *object,
                              guint property_id,
                              GValue *value,
                              GParamSpec *pspec) {
    UfoIrParallelProjectorTask *self = UFO_IR_PARALLEL_PROJECTOR_TASK(object);

    switch (property_id) {
        case PROP_MODEL:
            g_value_set_string (value, ufo_ir_parallel_projector_get_model(self));
            break;
        case PROP_ANGLES_NUM:
            g_value_set_uint(value, ufo_ir_parallel_projector_get_angles_num(self));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

/**
 * ufo_ir_parallel_projector_get_model:
 * @self: #UfoIrParallelProjectorTask
 *
 * Get projection model name
 *
 * Returns: projection model name
 */
const gchar *
ufo_ir_parallel_projector_get_model(UfoIrParallelProjectorTask *self) {
    UfoIrParallelProjectorTaskPrivate *priv = UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE(self);
    return priv->model_name;
}

/**
 * ufo_ir_parallel_projector_get_model:
 * @self: #UfoIrParallelProjectorTask
 * @model_name: new model name
 *
 * Set projection model name
 */
void
ufo_ir_parallel_projector_set_model(UfoIrParallelProjectorTask *self, const gchar *model_name) {
    UfoIrParallelProjectorTaskPrivate *priv = UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE(self);
    g_free(priv->model_name);
    priv->model_name = g_ascii_strdown(model_name, -1);
}

guint ufo_ir_parallel_projector_get_angles_num(UfoIrParallelProjectorTask *self) {
    UfoIrParallelProjectorTaskPrivate *priv = UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE(self);
    return priv->angles_num;
}

void ufo_ir_parallel_projector_set_angles_num(UfoIrParallelProjectorTask *self, guint angles_num) {
    UfoIrParallelProjectorTaskPrivate *priv = UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE(self);
    priv->angles_num = angles_num;
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ITask realization
// -----------------------------------------------------------------------------

static void
ufo_ir_parallel_projector_task_setup (UfoTask *self,
                                      UfoResources *resources,
                                      GError **error) {
    UfoIrParallelProjectorTaskPrivate *priv = UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE(self);

    priv->context = ufo_resources_get_context (resources);
    UFO_RESOURCES_CHECK_CLERR (clRetainContext (priv->context));

    // Load kernels
    gchar *filename = g_strconcat("projector-parallel-", priv->model_name, ".cl", NULL);
    gpointer *kernel[3] = { &priv->bp_kernel,
                            &priv->fp_kernel[Horizontal],
                            &priv->fp_kernel[Vertical]};

    *kernel[0] = ufo_resources_get_kernel (resources, filename, "BP", error);
    if (*error && error) {
        return;
    }

    *kernel[1] = ufo_resources_get_kernel (resources, filename, "FP_hor", error);
    if (*error && error) {
        return;
    }

    *kernel[2] = ufo_resources_get_kernel (resources, filename, "FP_vert", error);
    if (*error && error) {
        return;
    }

    priv->set_kernel = ufo_resources_get_kernel(resources, OPS_FILENAME, "operation_set", error);
    if (*error && error) {
        return;
    }
}

static void
ufo_ir_parallel_projector_task_get_requisition (UfoTask *self,
                                                UfoBuffer **inputs,
                                                UfoRequisition *requisition) {
    UfoIrParallelProjectorTaskPrivate *priv = UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE(self);

    UfoRequisition buffer_req;
    ufo_buffer_get_requisition(inputs[0], &buffer_req);
    if(ufo_ir_state_dependent_task_get_is_forward(UFO_IR_STATE_DEPENDENT_TASK(self))){
        if(priv->detectors_num != buffer_req.dims[0]){
            priv->detectors_num = buffer_req.dims[0];

            gfloat angles_step = ufo_ir_projector_task_get_step(UFO_IR_PROJECTOR_TASK(self));
            // Recalc sin/cos
            priv->scan_sin_lut = create_lut_buffer (priv->angles_num, angles_step, &priv->context, &priv->scan_host_sin_lut, sin);
            priv->scan_cos_lut = create_lut_buffer (priv->angles_num, angles_step, &priv->context, &priv->scan_host_cos_lut, cos);

            priv->full_subsets_list = generate_full_subsets_list(priv);
        }

        requisition->n_dims = 2;
        requisition->dims[0] = priv->detectors_num;
        requisition->dims[1] = priv->angles_num;
    }
    else{
        if(priv->detectors_num != buffer_req.dims[0] || priv->angles_num != buffer_req.dims[1]){
            priv->detectors_num = buffer_req.dims[0];
            priv->angles_num = buffer_req.dims[1];

            gfloat angles_step = ufo_ir_projector_task_get_step(UFO_IR_PROJECTOR_TASK(self));
            // Recalc sin/cos
            priv->scan_sin_lut = create_lut_buffer (priv->angles_num, angles_step, &priv->context, &priv->scan_host_sin_lut, sin);
            priv->scan_cos_lut = create_lut_buffer (priv->angles_num, angles_step, &priv->context, &priv->scan_host_cos_lut, cos);

            priv->full_subsets_list = generate_full_subsets_list(priv);
        }

        requisition->n_dims = 2;
        requisition->dims[0] = priv->detectors_num;
        requisition->dims[1] = priv->detectors_num;
    }
}

static UfoTaskMode
ufo_ir_parallel_projector_task_get_mode (UfoTask *task) {
    return UFO_TASK_MODE_PROCESSOR | UFO_TASK_MODE_GPU;
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// State dependent task implementation
// -----------------------------------------------------------------------------
gboolean
ufo_ir_parallel_projector_task_forward(UfoIrStateDependentTask *self,
                                       UfoBuffer **inputs,
                                       UfoBuffer *output,
                                       UfoRequisition *requisition) {
    UfoIrParallelProjectorTaskPrivate *priv = UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE(self);

    UfoGpuNode *node = UFO_GPU_NODE (ufo_task_node_get_proc_node (UFO_TASK_NODE (self)));
    cl_command_queue cmd_queue = ufo_gpu_node_get_cmd_queue (node);


    UfoRequisition req;
    ufo_buffer_get_requisition(output, &req);

    for (guint i = 0 ; i < priv->full_subsets_cnt; ++i) {
        ufo_ir_parallel_projector_subset_fp_real(UFO_IR_PARALLEL_PROJECTOR_TASK(self), inputs[0], output, &priv->full_subsets_list[i], &req, cmd_queue);
    }

    return TRUE;
}

gboolean
ufo_ir_parallel_projector_task_backward(UfoIrStateDependentTask *self,
                                        UfoBuffer **inputs,
                                        UfoBuffer *output,
                                        UfoRequisition *requisition) {
    UfoIrParallelProjectorTaskPrivate *priv = UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE(self);

    UfoGpuNode *node = UFO_GPU_NODE (ufo_task_node_get_proc_node (UFO_TASK_NODE (self)));
    cl_command_queue cmd_queue = ufo_gpu_node_get_cmd_queue (node);


    UfoRequisition req;
    ufo_buffer_get_requisition(output, &req);

    for (guint i = 0 ; i < priv->full_subsets_cnt; ++i) {
        ufo_ir_parallel_projector_subset_bp_real(UFO_IR_PARALLEL_PROJECTOR_TASK(self), output, inputs[0], &priv->full_subsets_list[i], &req, cmd_queue);
    }

    return TRUE;
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------------
static cl_mem
create_lut_buffer (guint angles_num,
                   gfloat angles_step,
                   const cl_context *context,
                   gfloat **host_mem,
                   double (*func)(double)) {
    cl_int errcode;
    gsize size = angles_num * sizeof (gfloat);
    cl_mem mem = NULL;

    *host_mem = (gfloat*)g_malloc0 (size);
    gdouble rad_angle = 0;
    for (guint i = 0; i < angles_num; i++) {
        rad_angle = i * angles_step;
        (*host_mem)[i] = (gfloat) func (rad_angle);
    }

    mem = clCreateBuffer (*context,
                          CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
                          size, *host_mem,
                          &errcode);

    UFO_RESOURCES_CHECK_CLERR (errcode);
    return mem;
}

static UfoIrProjectionsSubset *
generate_full_subsets_list (UfoIrParallelProjectorTaskPrivate *priv) {
    gfloat *sin_values = priv->scan_host_sin_lut;
    gfloat *cos_values = priv->scan_host_cos_lut;
    guint n_angles = priv->angles_num;

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

    priv->full_subsets_cnt = subset_index + 1;

    UfoIrProjectionsSubset *subsets =
        g_memdup (subsets_tmp, sizeof (UfoIrProjectionsSubset) * (priv->full_subsets_cnt));

    g_free (subsets_tmp);

    return subsets;
}

static void
ufo_ir_parallel_projector_subset_bp_real(UfoIrParallelProjectorTask *self,
                                         UfoBuffer *volume,
                                         UfoBuffer *sinogram,
                                         UfoIrProjectionsSubset *subset,
                                         UfoRequisition *requisitions,
                                         cl_command_queue cmd_queue) {
    UfoIrParallelProjectorTaskPrivate *priv = UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE(self);
    cl_kernel kernel = priv->bp_kernel;
    cl_mem d_volume = ufo_buffer_get_device_image (volume, cmd_queue);
    cl_mem d_sino = ufo_buffer_get_device_image (sinogram, cmd_queue);

    UfoIrProjectorTask *projection_task = UFO_IR_PROJECTOR_TASK(self);
    float relaxation = ufo_ir_projector_task_get_relaxation(projection_task);
    float axis_position = ufo_ir_projector_task_get_axis_position(projection_task);
    if(axis_position < 0)
    {
        axis_position = requisitions->dims[0] / 2.0;
    }

    /* Kernel definition
    void BP(__read_only  image2d_t           r_volume,      // 0
            __write_only image2d_t           w_volume,      // 1
            __read_only  image2d_t           sinogram,      // 2
            __const      float               relax_param,   // 3
            __constant   float               *sin_val,      // 4
            __constant   float               *cos_val,      // 5
            __const      float               axis_pos,      // 6
            __const      UfoProjectionsSubset    part)      // 7
            */
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof (cl_mem), &d_volume));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof (cl_mem), &d_volume));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof (cl_mem), &d_sino));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 3, sizeof (gfloat), &relaxation));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 4, sizeof (cl_mem), &priv->scan_sin_lut));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 5, sizeof (cl_mem), &priv->scan_cos_lut));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 6, sizeof (gfloat), &axis_position));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 7, sizeof (UfoIrProjectionsSubset), subset));

    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel(
                                   cmd_queue,           // cl_command_queue command_queue
                                   kernel,              // cl_kernel kernel
                                   requisitions->n_dims, // cl_uint work_dim
                                   NULL,                // const size_t *global_work_offset
                                   requisitions->dims,   // const size_t *global_work_size
                                   NULL,                // const size_t *local_work_size
                                   0,                   // cl_uint num_events_in_wait_list
                                   NULL,                // const cl_event *event_wait_list
                                   NULL));            // cl_event *event

}

static void
ufo_ir_parallel_projector_subset_fp_real(UfoIrParallelProjectorTask *self,
                                         UfoBuffer *volume,
                                         UfoBuffer *sinogram,
                                         UfoIrProjectionsSubset *subset,
                                         UfoRequisition *requisitions,
                                         cl_command_queue cmd_queue) {
    UfoIrParallelProjectorTaskPrivate *priv = UFO_IR_PARALLEL_PROJECTOR_TASK_GET_PRIVATE(self);
    cl_kernel kernel = priv->fp_kernel[subset->direction];

    cl_mem d_volume = ufo_buffer_get_device_image (volume, cmd_queue);
    cl_mem d_sinogram = ufo_buffer_get_device_image (sinogram, cmd_queue);

    UfoIrGeometryDims dims;
    dims.width = requisitions->dims[0];
    dims.height = requisitions->dims[0];
    dims.n_dets = requisitions->dims[0];
    dims.n_angles = requisitions->dims[1];

    UfoIrProjectorTask *projection_task = UFO_IR_PROJECTOR_TASK(self);
    float axis_position = ufo_ir_projector_task_get_axis_position(projection_task);
    float correction_scale = ufo_ir_projector_task_get_correction_scale(projection_task);
    if(axis_position < 0)
    {
        axis_position = requisitions->dims[0] / 2.0;
    }

    /* Kernel definition
    void FP(__read_only     image2d_t               volume,
            __read_only     image2d_t               r_sinogram,
            __write_only    image2d_t               w_sinogram,
            __constant      float                   *sin_val,
            __constant      float                   *cos_val,
            __const         UfoGeometryDims         dimensions,
            __const         float                   axis_pos,
            __const         UfoProjectionsSubset    part,
            __const         float                   correction_scale)
    */
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof (cl_mem), &d_volume));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof (cl_mem), &d_sinogram));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof (cl_mem), &d_sinogram));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 3, sizeof (cl_mem), &priv->scan_sin_lut));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 4, sizeof (cl_mem), &priv->scan_cos_lut));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 5, sizeof (UfoIrGeometryDims), &dims));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 6, sizeof (gfloat), &axis_position));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 7, sizeof (UfoIrProjectionsSubset), subset));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 8, sizeof (gfloat), &correction_scale));

//    UfoRequisition requisitions;
//    ufo_buffer_get_requisition (measurements, &requisitions);
    requisitions->dims[1] = subset->n;

    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel(
                                   cmd_queue,
                                   kernel,
                                   requisitions->n_dims,
                                   NULL,
                                   requisitions->dims,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL));

}
// -----------------------------------------------------------------------------
