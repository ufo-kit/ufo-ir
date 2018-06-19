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
static void twoAraysIterator(UfoIrBasicOpsProcessor *self, UfoBuffer *arg1, UfoBuffer *arg2, UfoBuffer *output, void (*proc_fn)(const float *, const float *, float *));

static void elementsMulOperation(const float *in1,const float *in2, float *outVal);
static void elementsMaxOperation(const float *in1,const float *in2, float *outVal);
static void elementsDivOperation(const float *in1,const float *in2, float *outVal);

struct _UfoIrBasicOpsProcessorPrivate {
    gpointer add_kernel;
    gpointer add2_kernel;
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

G_DEFINE_TYPE (UfoIrBasicOpsProcessor, ufo_ir_basic_ops_processor, G_TYPE_OBJECT)

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

static guint
num_elements (UfoRequisition *requisition)
{
    guint num = 1;

    for (guint dimension = 0; dimension < requisition->n_dims; dimension++) {
        num *= (guint)requisition->dims[dimension];
    }

    return num;
}

UfoIrBasicOpsProcessor *
ufo_ir_basic_ops_processor_new(UfoResources *resources,
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

gpointer
ufo_ir_basic_ops_processor_add (UfoIrBasicOpsProcessor *self,
                                UfoBuffer *buffer1,
                                UfoBuffer *buffer2,
                                UfoBuffer *result)
{
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    return operation (buffer1, buffer2, result, priv->command_queue, priv->add_kernel);
}

gpointer
ufo_ir_basic_ops_processor_add2 (UfoIrBasicOpsProcessor *self,
                                 UfoBuffer *buffer1,
                                 UfoBuffer *buffer2,
                                 gfloat modifier,
                                 UfoBuffer *result)
{
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    return operation2 (buffer1, buffer2, modifier, result, priv->command_queue, priv->add2_kernel);
}


gpointer
ufo_ir_basic_ops_processor_deduction (UfoIrBasicOpsProcessor *self,
                                      UfoBuffer *buffer1,
                                      UfoBuffer *buffer2,
                                      UfoBuffer *result)
{
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    return operation (buffer1, buffer2, result, priv->command_queue, priv->ded_kernel);
}

gpointer
ufo_ir_basic_ops_processor_deduction2 (UfoIrBasicOpsProcessor *self,
                                       UfoBuffer *buffer1,
                                       UfoBuffer *buffer2,
                                       gfloat modifier,
                                       UfoBuffer *result)
{
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    return operation2 (buffer1, buffer2, modifier, result, priv->command_queue, priv->ded2_kernel);
}

void
ufo_ir_basic_ops_processor_div_element_wise(UfoIrBasicOpsProcessor *self,
                                            UfoBuffer *buffer1,
                                            UfoBuffer *buffer2,
                                            UfoBuffer *result)
{
    twoAraysIterator(self, buffer1, buffer2, result, elementsDivOperation);
}

gfloat
ufo_ir_basic_ops_processor_dot_product(UfoIrBasicOpsProcessor *self,
                                       UfoBuffer *buffer1,
                                       UfoBuffer *buffer2)
{
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    UfoRequisition buffer1_requisition;
    UfoRequisition buffer2_requisition;

    ufo_buffer_get_requisition (buffer1, &buffer1_requisition);
    ufo_buffer_get_requisition (buffer2, &buffer2_requisition);

    gfloat *values1 = ufo_buffer_get_host_array (buffer1, priv->command_queue);
    gfloat *values2 = ufo_buffer_get_host_array (buffer2, priv->command_queue);

    guint length1 = num_elements (&buffer1_requisition);
    guint length2 = num_elements (&buffer2_requisition);

    guint length = 1;

    if (buffer1_requisition.n_dims != buffer2_requisition.n_dims) {
        g_print("Buffers are not equal\n");
        return -1.0f;
    }

    if (length1 == length2) {
        length = length1;
    }
    else {
        g_print("Buffers are not equal\n");
        return -1.0f;
    }

    guint partsCnt = (guint)buffer1_requisition.dims[buffer1_requisition.n_dims -1];
    guint partLen = length / partsCnt;

    gfloat norm = 0;

    for(guint partNum = 0; partNum < partsCnt; partNum++) {
        gfloat partNorm = 0;
        for(guint i = 0; i < partLen; i++) {
            guint index = partNum * i;
            partNorm += values1[index] * values2[index];
        }
        norm += partNorm;
    }

    return norm;
}

gpointer
ufo_ir_basic_ops_processor_inv (UfoIrBasicOpsProcessor *self,
                                UfoBuffer *buffer)
{
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

gfloat
ufo_ir_basic_ops_processor_l1_norm (UfoIrBasicOpsProcessor *self,
                                    UfoBuffer *buffer)
{
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

gfloat
ufo_ir_basic_ops_processor_l2_norm (UfoIrBasicOpsProcessor *self,
                                    UfoBuffer *buffer)
{
    gfloat norm;

    norm = ufo_ir_basic_ops_processor_dot_product (self, buffer, buffer);
    return sqrt (norm);
}

void
ufo_ir_basic_ops_processor_max_element_wise(UfoIrBasicOpsProcessor *self,
                                            UfoBuffer *buffer1,
                                            UfoBuffer *buffer2,
                                            UfoBuffer *result)
{
    twoAraysIterator(self, buffer1, buffer2, result, elementsMaxOperation);
}

gpointer
ufo_ir_basic_ops_processor_mul (UfoIrBasicOpsProcessor *self,
                                UfoBuffer *buffer1,
                                UfoBuffer *buffer2,
                                UfoBuffer *result)
{
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    return operation (buffer1, buffer2, result, priv->command_queue, priv->mul_kernel);
}

void
ufo_ir_basic_ops_processor_mul_element_wise(UfoIrBasicOpsProcessor *self,
                                            UfoBuffer *buffer1,
                                            UfoBuffer *buffer2,
                                            UfoBuffer *result)
{
    twoAraysIterator(self, buffer1, buffer2, result, elementsMulOperation);
}

gpointer
ufo_ir_basic_ops_processor_mul_rows (UfoIrBasicOpsProcessor *self,
                                     UfoBuffer *buffer1,
                                     UfoBuffer *buffer2,
                                     UfoBuffer *result,
                                     guint offset,
                                     guint n)
{
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

void
ufo_ir_basic_ops_processor_mul_scalar(UfoIrBasicOpsProcessor *self,
                                      UfoBuffer *buffer,
                                      gfloat multiplier)
{
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    UfoRequisition requisition;
    ufo_buffer_get_requisition (buffer, &requisition);

    gfloat *values = ufo_buffer_get_host_array (buffer, priv->command_queue);

    guint length = num_elements (&requisition);

    for(guint i = 0; i < length; i++) {
        values[i] *= multiplier;
    }
}

void
ufo_ir_basic_ops_processor_normalization(UfoIrBasicOpsProcessor *self,
                                         UfoBuffer *buffer)
{
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    UfoRequisition requisition;
    ufo_buffer_get_requisition (buffer, &requisition);

    gfloat *values = ufo_buffer_get_host_array (buffer, priv->command_queue);

    guint length = num_elements (&requisition);

    gfloat max = -G_MAXFLOAT;
    gfloat min =  G_MAXFLOAT;

    for (guint i = 0; i < length; i++) {
        max = fmax(values[i], max);
        min = fmin(values[i], min);
    }

    gfloat delta = 1 / (max - min);

    for (guint i = 0; i < length; i++) {
        values[i] = delta * values[i] - min * delta;
    }
}

gpointer
ufo_ir_basic_ops_processor_positive_constraint (UfoIrBasicOpsProcessor *self,
                                                UfoBuffer *buffer,
                                                UfoBuffer *result)
{
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

gpointer
ufo_ir_basic_ops_processor_set (UfoIrBasicOpsProcessor *self,
                                UfoBuffer *buffer,
                                gfloat value)
{
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

void
ufo_ir_basic_ops_processor_sqrt (UfoIrBasicOpsProcessor *self,
                                 UfoBuffer *buffer)
{
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    UfoRequisition arg_requisition;
    ufo_buffer_get_requisition (buffer, &arg_requisition);

    gfloat *values = ufo_buffer_get_host_array (buffer, priv->command_queue);

    guint length = num_elements (&arg_requisition);

    for(guint i = 0; i < length; i++) {
        values[i] =  sqrt(values[i]);
    }
}

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
kernel_from_name (UfoResources *resources,
                  const gchar* name)
{
    GError *error = NULL;
    gpointer kernel = ufo_resources_get_kernel (resources, OPS_FILENAME, name, NULL, &error);

    if (error) {
        g_error ("%s\n", error->message);
        g_error_free (error);
        return NULL;
    }

    return kernel;
}

static void ufo_ir_basic_obs_processor_resources_init(UfoIrBasicOpsProcessor *self,
                                                      UfoResources *resources,
                                                      cl_command_queue cmd_queue)
{
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);

    priv->command_queue = cmd_queue;
    priv->resources = resources;

    priv->add_kernel = kernel_from_name(resources, "operation_add");
    priv->add2_kernel = kernel_from_name(resources, "operation_add2");
    priv->ded_kernel = kernel_from_name(resources, "operation_deduction");
    priv->ded2_kernel = kernel_from_name(resources, "operation_deduction2");
    priv->inv_kernel =  kernel_from_name(resources, "operation_inv");
    priv->mul_kernel = kernel_from_name(resources, "operation_mul");
    priv->mul_rows_kernel = kernel_from_name(resources, "op_mulRows");
    priv->pc_kernel = kernel_from_name(resources, "POSC");
    priv->set_kernel = kernel_from_name(resources, "operation_set");
}

static void
twoAraysIterator(UfoIrBasicOpsProcessor *self,
                 UfoBuffer *arg1,
                 UfoBuffer *arg2,
                 UfoBuffer *output,
                 void(*proc_fn)(const float*, const float*, float*))
{
    UfoIrBasicOpsProcessorPrivate *priv = UFO_IR_BASIC_OPS_PROCESSOR_GET_PRIVATE(self);
    UfoRequisition arg1_requisition;
    UfoRequisition arg2_requisition;

    ufo_buffer_get_requisition (arg1, &arg1_requisition);
    ufo_buffer_get_requisition (arg2, &arg2_requisition);

    gfloat *values1 = ufo_buffer_get_host_array (arg1, priv->command_queue);
    gfloat *values2 = ufo_buffer_get_host_array (arg2, priv->command_queue);

    guint length1 = num_elements (&arg1_requisition);
    guint length2 = num_elements (&arg2_requisition);
    guint length = 1;

    if (arg1_requisition.n_dims != arg2_requisition.n_dims) {
        g_print("Buffers are not equal\n");
    }

    length = length1;

    if (length1 != length2) {
        g_print("Buffers are not equal\n");
    }

    gfloat *outputs = ufo_buffer_get_host_array (output, priv->command_queue);

    for (guint i = 0; i < length; i++) {
        proc_fn(&values1[i], &values2[i], &outputs[i]);
    }
}

static void
elementsMulOperation(const float *in1,
                     const float *in2,
                     float *outVal)
{
    *outVal = (*in1) * (*in2);
}

static void
elementsMaxOperation(const float *in1,
                     const float *in2,
                     float *outVal)
{
    *outVal = fmax(*in1,*in2);
}

static void
elementsDivOperation(const float *in1,
                     const float *in2,
                     float *outVal)
{
    *outVal = (*in1) / (*in2);
}
