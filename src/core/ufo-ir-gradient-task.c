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

#include "ufo-ir-gradient-task.h"

#define KERNELS_FILE_NAME "sb-gradient.cl"

struct _UfoIrGradientTaskPrivate {
    cl_kernel dxKernel;
    cl_kernel dyKernel;
    cl_kernel dxtKernel;
    cl_kernel dytKernel;
};

static void ufo_task_interface_init (UfoTaskIface *iface);
static UfoTaskMode ufo_ir_gradient_task_get_mode (UfoTask *task);
static void ufo_ir_gradient_task_setup (UfoTask *task, UfoResources *resources,GError **error);
static guint ufo_ir_gradient_task_get_num_inputs (UfoTask *task);
static guint ufo_ir_gradient_task_get_num_dimensions (UfoTask *task, guint input);
static void ufo_ir_gradient_task_get_requisition (UfoTask *task, UfoBuffer **inputs, UfoRequisition *requisition);
static void ufo_ir_gradient_task_finalize (GObject *object);
gboolean ufo_ir_gradient_task_forward(UfoIrStateDependentTask *self, UfoBuffer **inputs, UfoBuffer *output, UfoRequisition *requisition);
gboolean ufo_ir_gradient_task_backward(UfoIrStateDependentTask *self, UfoBuffer **inputs, UfoBuffer *output, UfoRequisition *requisition);
static void dx_op  (UfoIrGradientTaskPrivate *priv, UfoBuffer *input, UfoBuffer *output, cl_command_queue command_queue, UfoRequisition *requisition);
static void dxt_op (UfoIrGradientTaskPrivate *priv, UfoBuffer *input, UfoBuffer *output, cl_command_queue command_queue, UfoRequisition *requisition);
static void dy_op  (UfoIrGradientTaskPrivate *priv, UfoBuffer *input, UfoBuffer *output, cl_command_queue command_queue, UfoRequisition *requisition);
static void dyt_op (UfoIrGradientTaskPrivate *priv, UfoBuffer *input, UfoBuffer *output, cl_command_queue command_queue, UfoRequisition *requisition);

G_DEFINE_TYPE_WITH_CODE (UfoIrGradientTask, ufo_ir_gradient_task, UFO_IR_TYPE_STATE_DEPENDENT_TASK,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_IR_GRADIENT_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_GRADIENT_TASK, UfoIrGradientTaskPrivate))

// -----------------------------------------------------------------------------
// Init methods
// -----------------------------------------------------------------------------
static void
ufo_task_interface_init (UfoTaskIface *iface) {
    iface->setup = ufo_ir_gradient_task_setup;
    iface->get_num_inputs = ufo_ir_gradient_task_get_num_inputs;
    iface->get_num_dimensions = ufo_ir_gradient_task_get_num_dimensions;
    iface->get_mode = ufo_ir_gradient_task_get_mode;
    iface->get_requisition = ufo_ir_gradient_task_get_requisition;
}

static void
ufo_ir_gradient_task_class_init (UfoIrGradientTaskClass *klass) {

    GObjectClass *oclass = G_OBJECT_CLASS (klass);
    oclass->finalize = ufo_ir_gradient_task_finalize;

    UfoIrStateDependentTaskClass *sdclass = UFO_IR_STATE_DEPENDENT_TASK_CLASS(klass);
    sdclass->forward = ufo_ir_gradient_task_forward;
    sdclass->backward = ufo_ir_gradient_task_backward;

    g_type_class_add_private (oclass, sizeof(UfoIrGradientTaskPrivate));
}

static void
ufo_ir_gradient_task_init(UfoIrGradientTask *self) {
    self->priv = UFO_IR_GRADIENT_TASK_GET_PRIVATE(self);
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// GObject methods realization
// -----------------------------------------------------------------------------
static void
ufo_ir_gradient_task_finalize (GObject *object) {
    G_OBJECT_CLASS (ufo_ir_gradient_task_parent_class)->finalize (object);
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ITask methods realization
// -----------------------------------------------------------------------------
UfoNode *
ufo_ir_gradient_task_new (void) {
    return UFO_NODE (g_object_new (UFO_IR_TYPE_GRADIENT_TASK, NULL));
}

static void
ufo_ir_gradient_task_setup (UfoTask *task,
                            UfoResources *resources,
                            GError **error) {

    UfoIrGradientTaskPrivate *priv = UFO_IR_GRADIENT_TASK_GET_PRIVATE(task);

    priv->dxKernel = ufo_resources_get_kernel(resources, KERNELS_FILE_NAME,"Dx", error);
    if (priv->dxKernel != NULL) {
            UFO_RESOURCES_CHECK_CLERR (clRetainKernel (priv->dxKernel));
    }

    priv->dyKernel = ufo_resources_get_kernel(resources, KERNELS_FILE_NAME,"Dy", error);
    if (priv->dyKernel != NULL) {
            UFO_RESOURCES_CHECK_CLERR (clRetainKernel (priv->dyKernel));
    }

    priv->dxtKernel = ufo_resources_get_kernel(resources, KERNELS_FILE_NAME,"Dxt", error);
    if (priv->dxtKernel != NULL) {
            UFO_RESOURCES_CHECK_CLERR (clRetainKernel (priv->dxtKernel));
    }

    priv->dytKernel = ufo_resources_get_kernel(resources, KERNELS_FILE_NAME,"Dyt", error);
    if (priv->dytKernel != NULL) {
            UFO_RESOURCES_CHECK_CLERR (clRetainKernel (priv->dytKernel));
    }
}

static void
ufo_ir_gradient_task_get_requisition (UfoTask *task,
                                      UfoBuffer **inputs,
                                      UfoRequisition *requisition) {
    UfoRequisition buffer_req;
    ufo_buffer_get_requisition(inputs[0], &buffer_req);

    requisition->dims[0] = buffer_req.dims[0];
    requisition->dims[1] = buffer_req.dims[1];
    requisition->n_dims = 2;
}

static guint
ufo_ir_gradient_task_get_num_inputs (UfoTask *task) {
    return 1;
}

static guint
ufo_ir_gradient_task_get_num_dimensions (UfoTask *task, guint input) {
    return 2;
}

static UfoTaskMode
ufo_ir_gradient_task_get_mode (UfoTask *task) {
    return UFO_TASK_MODE_PROCESSOR | UFO_TASK_MODE_GPU;
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// StateDependentTask methods realization
// -----------------------------------------------------------------------------
gboolean
ufo_ir_gradient_task_forward(UfoIrStateDependentTask *self, UfoBuffer **inputs, UfoBuffer *output, UfoRequisition *requisition) {

}

gboolean
ufo_ir_gradient_task_backward(UfoIrStateDependentTask *self, UfoBuffer **inputs, UfoBuffer *output, UfoRequisition *requisition) {

}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------------
static void
dx_op (UfoIrGradientTaskPrivate *priv,
       UfoBuffer *input,
       UfoBuffer *output,
       cl_command_queue command_queue,
       UfoRequisition *requisition) {
    cl_kernel kernel = priv->dxKernel;
    cl_mem d_input = ufo_buffer_get_device_array(input, command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, command_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       requisition->n_dims, NULL, requisition->dims,
                                                       NULL, 0, NULL, NULL));
}

static void
dxt_op (UfoIrGradientTaskPrivate *priv,
        UfoBuffer *input,
        UfoBuffer *output,
        cl_command_queue command_queue,
        UfoRequisition *requisition) {
    cl_kernel kernel = priv->dxtKernel;
    cl_mem d_input = ufo_buffer_get_device_array(input, command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, command_queue);
    int stopIndex = requisition->dims[0] - 1;

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(int), (void *) &stopIndex));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       requisition->n_dims, NULL, requisition->dims,
                                                       NULL, 0, NULL, NULL));
}

static void
dy_op (UfoIrGradientTaskPrivate *priv,
       UfoBuffer *input,
       UfoBuffer *output,
       cl_command_queue command_queue,
       UfoRequisition *requisition) {
    cl_kernel kernel = priv->dyKernel;
    cl_mem d_input = ufo_buffer_get_device_array(input, command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, command_queue);
    int lastOffset = requisition->dims[0] * requisition->dims[1];

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(int), (void *) &lastOffset));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       requisition->n_dims, NULL, requisition->dims,
                                                       NULL, 0, NULL, NULL));
}

static void
dyt_op (UfoIrGradientTaskPrivate *priv,
        UfoBuffer *input,
        UfoBuffer *output,
        cl_command_queue command_queue,
        UfoRequisition *requisition) {
    cl_kernel kernel = priv->dytKernel;
    cl_mem d_input = ufo_buffer_get_device_array(input, command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, command_queue);
    gint lastOffset = requisition->dims[0] * requisition->dims[1];
    gint stopIndex = requisition->dims[1] - 1;

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(gint), (void *) &lastOffset));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof(gint), (void *) &stopIndex));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 3, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       requisition->n_dims, NULL, requisition->dims,
                                                       NULL, 0, NULL, NULL));
}
// -----------------------------------------------------------------------------
