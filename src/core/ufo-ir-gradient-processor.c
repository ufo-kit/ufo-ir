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

#include "ufo-ir-gradient-processor.h"

#define KERNELS_FILE_NAME "ufo-ir-gradient-processor.cl"

struct _UfoIrGradientProcessorPrivate {
    // Kernels
    cl_kernel dxKernel;
    cl_kernel dyKernel;
    cl_kernel dxtKernel;
    cl_kernel dytKernel;

    // Useful things
    UfoResources *resources;
    cl_command_queue command_queue;
};

static void ufo_ir_gradient_processor_finalize (GObject *object);
static void ufo_ir_gradient_processor_resources_init(UfoIrGradientProcessor *self, UfoResources *resources, cl_command_queue cmd_queue);

G_DEFINE_TYPE (UfoIrGradientProcessor, ufo_ir_gradient_processor, G_TYPE_OBJECT)

#define UFO_IR_GRADIENT_PROCESSOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_GRADIENT_PROCESSOR, UfoIrGradientProcessorPrivate))

static void
ufo_ir_gradient_processor_class_init (UfoIrGradientProcessorClass *klass)
{

    GObjectClass *oclass = G_OBJECT_CLASS (klass);
    oclass->finalize = ufo_ir_gradient_processor_finalize;

    g_type_class_add_private (oclass, sizeof(UfoIrGradientProcessorPrivate));
}

static void
ufo_ir_gradient_processor_init(UfoIrGradientProcessor *self)
{
    self->priv = UFO_IR_GRADIENT_PROCESSOR_GET_PRIVATE(self);
}

UfoIrGradientProcessor *
ufo_ir_gradient_processor_new (UfoResources *resources, cl_command_queue cmd_queue)
{
    UfoIrGradientProcessor *processor = UFO_IR_GRADIENT_PROCESSOR (g_object_new (UFO_IR_TYPE_GRADIENT_PROCESSOR, NULL));
    ufo_ir_gradient_processor_resources_init(processor, resources, cmd_queue);
    return processor;
}

static void
ufo_ir_gradient_processor_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_ir_gradient_processor_parent_class)->finalize (object);
}

void
ufo_ir_gradient_processor_dx_op (UfoIrGradientProcessor *self,
                                 UfoBuffer *input,
                                 UfoBuffer *output)
{
    UfoIrGradientProcessorPrivate *priv = UFO_IR_GRADIENT_PROCESSOR_GET_PRIVATE(self);
    UfoRequisition requisition;
    ufo_buffer_get_requisition(input,&requisition);
    cl_kernel kernel = priv->dxKernel;
    cl_mem d_input = ufo_buffer_get_device_array(input, priv->command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, priv->command_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (priv->command_queue, kernel,
                                                       requisition.n_dims, NULL, requisition.dims,
                                                       NULL, 0, NULL, NULL));
}

void
ufo_ir_gradient_processor_dxt_op (UfoIrGradientProcessor *self,
                                  UfoBuffer *input,
                                  UfoBuffer *output)
{
    UfoIrGradientProcessorPrivate *priv = UFO_IR_GRADIENT_PROCESSOR_GET_PRIVATE(self);
    UfoRequisition requisition;
    ufo_buffer_get_requisition(input,&requisition);
    cl_kernel kernel = priv->dxtKernel;
    cl_mem d_input = ufo_buffer_get_device_array(input, priv->command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, priv->command_queue);
    int stopIndex = requisition.dims[0] - 1;

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(int), (void *) &stopIndex));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (priv->command_queue, kernel,
                                                       requisition.n_dims, NULL, requisition.dims,
                                                       NULL, 0, NULL, NULL));
}

void
ufo_ir_gradient_processor_dy_op (UfoIrGradientProcessor *self,
                                 UfoBuffer *input,
                                 UfoBuffer *output)
{
    UfoIrGradientProcessorPrivate *priv = UFO_IR_GRADIENT_PROCESSOR_GET_PRIVATE(self);
    UfoRequisition requisition;
    ufo_buffer_get_requisition(input,&requisition);
    cl_kernel kernel = priv->dyKernel;
    cl_mem d_input = ufo_buffer_get_device_array(input, priv->command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, priv->command_queue);
    int lastOffset = requisition.dims[0] * requisition.dims[1];

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(int), (void *) &lastOffset));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (priv->command_queue, kernel,
                                                       requisition.n_dims, NULL, requisition.dims,
                                                       NULL, 0, NULL, NULL));
}

void
ufo_ir_gradient_processor_dyt_op (UfoIrGradientProcessor *self,
                                  UfoBuffer *input,
                                  UfoBuffer *output)
{
    UfoIrGradientProcessorPrivate *priv = UFO_IR_GRADIENT_PROCESSOR_GET_PRIVATE(self);
    UfoRequisition requisition;
    ufo_buffer_get_requisition(input,&requisition);
    cl_kernel kernel = priv->dytKernel;
    cl_mem d_input = ufo_buffer_get_device_array(input, priv->command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, priv->command_queue);
    gint lastOffset = requisition.dims[0] * requisition.dims[1];
    gint stopIndex = requisition.dims[1] - 1;

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(gint), (void *) &lastOffset));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof(gint), (void *) &stopIndex));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 3, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (priv->command_queue, kernel,
                                                       requisition.n_dims, NULL, requisition.dims,
                                                       NULL, 0, NULL, NULL));
}

static void
ufo_ir_gradient_processor_resources_init(UfoIrGradientProcessor *self,
                                         UfoResources *resources,
                                         cl_command_queue cmd_queue)
{
    UfoIrGradientProcessorPrivate *priv = UFO_IR_GRADIENT_PROCESSOR_GET_PRIVATE(self);

    priv->command_queue = cmd_queue;

    priv->dxKernel = ufo_resources_get_kernel(resources, KERNELS_FILE_NAME,"Dx", NULL);
    if (priv->dxKernel != NULL) {
            UFO_RESOURCES_CHECK_CLERR (clRetainKernel (priv->dxKernel));
    }

    priv->dyKernel = ufo_resources_get_kernel(resources, KERNELS_FILE_NAME,"Dy", NULL);
    if (priv->dyKernel != NULL) {
            UFO_RESOURCES_CHECK_CLERR (clRetainKernel (priv->dyKernel));
    }

    priv->dxtKernel = ufo_resources_get_kernel(resources, KERNELS_FILE_NAME,"Dxt", NULL);
    if (priv->dxtKernel != NULL) {
            UFO_RESOURCES_CHECK_CLERR (clRetainKernel (priv->dxtKernel));
    }

    priv->dytKernel = ufo_resources_get_kernel(resources, KERNELS_FILE_NAME,"Dyt", NULL);
    if (priv->dytKernel != NULL) {
            UFO_RESOURCES_CHECK_CLERR (clRetainKernel (priv->dytKernel));
    }
}
