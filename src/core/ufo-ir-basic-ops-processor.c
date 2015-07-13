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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include "ufo-ir-basic-ops-processor.h"
#define OPS_FILENAME "ufo-ir-basic-ops.cl"

static cl_event operation (UfoBuffer *arg1, UfoBuffer *arg2, UfoBuffer *out, gpointer command_queue, gpointer kernel);
static cl_event operation2 (UfoBuffer *arg1, UfoBuffer *arg2, gfloat modifier, UfoBuffer *out, gpointer command_queue, gpointer kernel);
static gpointer kernel_from_name(UfoResources *resources, const gchar* name);
static void ufo_ir_basic_obs_processor_resources_init(UfoIrBasicOpsProcessor *self, UfoResources *resources, cl_command_queue cmd_queue);
static void ufo_ir_basic_ops_processor_finalize (GObject *object);

struct _UfoIrBasicOpsProcessorPrivate {
    gpointer add_kernel;
    gpointer ded_kernel;
    gpointer ded2_kernel;
    gpointer inv_kernel;
    gpointer mul_kernel;
    gpointer mul_rows_kernel;
    gpointer pc_kernel;
    gpointer set_kernel;

    // Useful things
    UfoResources *resources;
    cl_command_queue command_queue;
};

G_DEFINE_TYPE (UfoIrBasicOpsProcessor, ufo_ir_basic_ops_processor, UFO_TYPE_TASK_NODE)

#define UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_BASIC_OPS_PROCESSOR, UfoIrBasicOpsProcessorPrivate))

static void
ufo_ir_basic_ops_processor_class_init (UfoIrBasicOpsProcessorClass *klass)
{
    GObjectClass *oclass = G_OBJECT_CLASS (klass);
    oclass->finalize = ufo_ir_basic_ops_processor_finalize;

    g_type_class_add_private (oclass, sizeof(UfoIrBasicOpsProcessorPrivate));
}

static void
ufo_ir_basic_ops_processor_init(UfoIrBasicOpsProcessor *self)
{
    self->priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
}

UfoIrBasicOpsProcessor *ufo_ir_basic_ops_processor_new(UfoResources *resources,
                                                       cl_command_queue cmd_queue) {
    UfoIrBasicOpsProcessor *processor = UFO_IR_BASIC_OPS_PROCESSOR (g_object_new (UFO_IR_TYPE_BASIC_OPS_PROCESSOR, NULL));
    ufo_ir_basic_obs_processor_resources_init(processor, resources, cmd_queue);
    return processor;
}

static void
ufo_ir_basic_ops_processor_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_ir_basic_ops_processor_parent_class)->finalize (object);
}

// -----------------------------------------------------------------------------
// Public methods
// -----------------------------------------------------------------------------
gpointer ufo_ir_basic_ops_processor_add (UfoIrBasicOpsProcessor *self,
                                         UfoBuffer *buffer1,
                                         UfoBuffer *buffer2,
                                         UfoBuffer *result) {
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    return operation (buffer1, buffer2, result, priv->command_queue, priv->add_kernel);
}

gpointer ufo_ir_basic_ops_processor_deduction (UfoIrBasicOpsProcessor *self,
                                               UfoBuffer *buffer1,
                                               UfoBuffer *buffer2,
                                               UfoBuffer *result) {
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    return operation (buffer1, buffer2, result, priv->command_queue, priv->ded_kernel);
}

gpointer ufo_ir_basic_ops_processor_deduction2 (UfoIrBasicOpsProcessor *self,
                                                UfoBuffer *buffer1,
                                                UfoBuffer *buffer2,
                                                gfloat modifier,
                                                UfoBuffer *result) {
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    return operation2 (buffer1, buffer2, modifier, result, priv->command_queue, priv->ded2_kernel);

}

gpointer ufo_ir_basic_ops_processor_inv (UfoIrBasicOpsProcessor *self,
                                         UfoBuffer *buffer) {
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    UfoRequisition requisition;
    ufo_buffer_get_requisition (buffer, &requisition);

    cl_mem d_arg = ufo_buffer_get_device_image (buffer, priv->command_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg(priv->inv_kernel, 0, sizeof(void *), (void *) &d_arg));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg(priv->inv_kernel, 1, sizeof(void *), (void *) &d_arg));
    cl_event event;
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel(priv->command_queue, priv->inv_kernel,
                                                      requisition.n_dims, NULL, requisition.dims,
                                                      NULL, 0, NULL, &event));

    return event;
}

gfloat ufo_ir_basic_ops_processor_l1_norm (UfoIrBasicOpsProcessor *self,
                                           UfoBuffer *buffer) {
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    UfoRequisition arg_requisition;
    gfloat *values;
    gfloat norm = 0;

    ufo_buffer_get_requisition (buffer, &arg_requisition);
    values = ufo_buffer_get_host_array (buffer, priv->command_queue);

    for (guint i = 0; i < arg_requisition.dims[0]; ++i) {
        for (guint j = 0; j < arg_requisition.dims[1]; ++j) {
            norm += (gfloat) fabs (values[i * arg_requisition.dims[1] + j]);
        }
    }

    return norm;
}

gpointer ufo_ir_basic_ops_processor_mul (UfoIrBasicOpsProcessor *self,
                                         UfoBuffer *buffer1,
                                         UfoBuffer *buffer2,
                                         UfoBuffer *result) {
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    return operation (buffer1, buffer2, result, priv->command_queue, priv->mul_kernel);
}

gpointer ufo_ir_basic_ops_processor_mul_rows (UfoIrBasicOpsProcessor *self,
                                              UfoBuffer *buffer1,
                                              UfoBuffer *buffer2,
                                              UfoBuffer *result,
                                              guint offset,
                                              guint n) {
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    UfoRequisition buffer1_requisition, buffer2_requisition, result_requisition;
    ufo_buffer_get_requisition (buffer1, &buffer1_requisition);
    ufo_buffer_get_requisition (buffer2, &buffer2_requisition);
    ufo_buffer_get_requisition (result, &result_requisition);

    if (buffer1_requisition.dims[0] != buffer2_requisition.dims[0] ||
        buffer1_requisition.dims[0] != result_requisition.dims[0]) {
        g_error ("Number of columns is different.");
        return NULL;
    }

    if (buffer1_requisition.dims[1] < offset + n ||
        buffer2_requisition.dims[1] < offset + n ||
        result_requisition.dims[1] < offset + n) {
        g_error ("Rows are not enough.");
        return NULL;
    }

    cl_mem d_buffer1 = ufo_buffer_get_device_image (buffer1, priv->command_queue);
    cl_mem d_buffer2 = ufo_buffer_get_device_image (buffer2, priv->command_queue);
    cl_mem d_result  = ufo_buffer_get_device_image (result, priv->command_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (priv->mul_rows_kernel, 0, sizeof(void *), (void *) &d_buffer1));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (priv->mul_rows_kernel, 1, sizeof(void *), (void *) &d_buffer2));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (priv->mul_rows_kernel, 2, sizeof(void *), (void *) &d_result));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (priv->mul_rows_kernel, 3, sizeof(unsigned int), (void *) &offset));

    UfoRequisition operation_requisition = result_requisition;
    operation_requisition.dims[1] = n;

    cl_event event;
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (priv->command_queue, priv->mul_rows_kernel,
                                                       operation_requisition.n_dims, NULL, operation_requisition.dims,
                                                       NULL, 0, NULL, &event));

    return event;
}

gpointer ufo_ir_basic_ops_processor_positive_constraint (UfoIrBasicOpsProcessor *self,
                                                         UfoBuffer *buffer,
                                                         UfoBuffer *result) {
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    UfoRequisition buffer_requisition;
    cl_event event;

    ufo_buffer_get_requisition (buffer, &buffer_requisition);
    ufo_buffer_resize (result, &buffer_requisition);

    cl_mem d_buffer = ufo_buffer_get_device_image (buffer, priv->command_queue);
    cl_mem d_result = ufo_buffer_get_device_image (result, priv->command_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (priv->pc_kernel, 0, sizeof(void *), (void *) &d_buffer));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (priv->pc_kernel, 1, sizeof(void *), (void *) &d_result));

    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (priv->command_queue, priv->pc_kernel,
                                                       buffer_requisition.n_dims, NULL, buffer_requisition.dims,
                                                       NULL, 0, NULL, &event));

    return event;
}

gpointer ufo_ir_basic_ops_processor_set (UfoIrBasicOpsProcessor *self,
                                         UfoBuffer *buffer,
                                         gfloat value) {
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    UfoRequisition requisition;
    ufo_buffer_get_requisition (buffer, &requisition);
    cl_mem d_buffer = ufo_buffer_get_device_image (buffer, priv->command_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (priv->set_kernel, 0, sizeof(void *), (void *) &d_buffer));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (priv->set_kernel, 1, sizeof(gfloat), (void *) &value));

    cl_event event;
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (priv->command_queue, priv->set_kernel,
                                                       requisition.n_dims, NULL, requisition.dims,
                                                       NULL, 0, NULL, &event));

    return event;
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------------
static cl_event
operation (UfoBuffer *arg1,
           UfoBuffer *arg2,
           UfoBuffer *out,
           gpointer command_queue,
           gpointer kernel)
{
    UfoRequisition arg1_requisition, arg2_requisition, out_requisition;
    cl_event event;

    ufo_buffer_get_requisition (arg1, &arg1_requisition);
    ufo_buffer_get_requisition (arg2, &arg2_requisition);
    ufo_buffer_get_requisition (out, &out_requisition);

    if ((arg1_requisition.dims[0] != arg2_requisition.dims[0] &&
         arg1_requisition.dims[0] != out_requisition.dims[0]) ||
        (arg1_requisition.dims[1] != arg2_requisition.dims[1] &&
         arg1_requisition.dims[1] != out_requisition.dims[1])) {
        g_error ("Incorrect volume size.");
        return NULL;
    }

    cl_mem d_arg1 = ufo_buffer_get_device_image (arg1, command_queue);
    cl_mem d_arg2 = ufo_buffer_get_device_image (arg2, command_queue);
    cl_mem d_out = ufo_buffer_get_device_image (out, command_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_arg1));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(void *), (void *) &d_arg2));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof(void *), (void *) &d_out));

    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       arg1_requisition.n_dims, NULL, arg1_requisition.dims,
                                                       NULL, 0, NULL, &event));

    return event;
}

static cl_event
operation2 (UfoBuffer *arg1,
            UfoBuffer *arg2,
            gfloat modifier,
            UfoBuffer *out,
            gpointer command_queue,
            gpointer kernel)
{
    UfoRequisition arg1_requisition, arg2_requisition, out_requisition;
    cl_event event;

    ufo_buffer_get_requisition (arg1, &arg1_requisition);
    ufo_buffer_get_requisition (arg2, &arg2_requisition);
    ufo_buffer_get_requisition (out, &out_requisition);

    if ((arg1_requisition.dims[0] != arg2_requisition.dims[0] &&
         arg1_requisition.dims[0] != out_requisition.dims[0]) ||
        (arg1_requisition.dims[1] != arg2_requisition.dims[1] &&
         arg1_requisition.dims[1] != out_requisition.dims[1])) {
        g_error ("Incorrect volume size.");
        return NULL;
    }

    cl_mem d_arg1 = ufo_buffer_get_device_image (arg1, command_queue);
    cl_mem d_arg2 = ufo_buffer_get_device_image (arg2, command_queue);
    cl_mem d_out = ufo_buffer_get_device_image (out, command_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg(kernel, 0, sizeof(void *), (void *) &d_arg1));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg(kernel, 1, sizeof(void *), (void *) &d_arg2));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg(kernel, 2, sizeof(gfloat), (void *) &modifier));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg(kernel, 3, sizeof(void *), (void *) &d_out));

    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       arg1_requisition.n_dims, NULL, arg1_requisition.dims,
                                                       NULL, 0, NULL, &event));

    return event;
}

static gpointer
kernel_from_name(UfoResources *resources,
                 const gchar* name) {
    GError *error = NULL;
    gpointer kernel = ufo_resources_get_kernel (resources, OPS_FILENAME, name, &error);
    if (error) {
        g_error ("%s\n", error->message);
        return NULL;
    }

    return kernel;
}

static void ufo_ir_basic_obs_processor_resources_init(UfoIrBasicOpsProcessor *self,
                                                      UfoResources *resources,
                                                      cl_command_queue cmd_queue) {
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);

    priv->command_queue = cmd_queue;
    priv->resources = resources;

    priv->add_kernel = kernel_from_name(resources, "operation_add");
    priv->ded_kernel = kernel_from_name(resources, "operation_deduction");
    priv->ded2_kernel = kernel_from_name(resources, "operation_deduction2");
    priv->inv_kernel =  kernel_from_name(resources, "operation_inv");
    priv->mul_kernel = kernel_from_name(resources, "operation_mul");
    priv->mul_rows_kernel = kernel_from_name(resources, "op_mulRows");
    priv->pc_kernel = kernel_from_name(resources, "POSC");
    priv->set_kernel = kernel_from_name(resources, "operation_set");
}
// -----------------------------------------------------------------------------
