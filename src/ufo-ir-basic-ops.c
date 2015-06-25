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

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <math.h>
#include "ufo-ir-basic-ops.h"
#define OPS_FILENAME "ufo-basic-ops.cl"

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

static gpointer
kernel_from_name(UfoResources *resources, const gchar* name) {
    GError *error = NULL;
    gpointer kernel = ufo_resources_get_kernel (resources, OPS_FILENAME, name, &error);
    if (error) {
        g_error ("%s\n", error->message);
        return NULL;
    }

    return kernel;
}

gpointer
ufo_ir_op_set (UfoBuffer *arg,
               gfloat     value,
               gpointer   command_queue,
               gpointer   kernel) {
    UfoRequisition requisition;
    ufo_buffer_get_requisition (arg, &requisition);
    cl_mem d_arg = ufo_buffer_get_device_image (arg, command_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_arg));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(gfloat), (void *) &value));

    cl_event event;
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       requisition.n_dims, NULL, requisition.dims,
                                                       NULL, 0, NULL, &event));

    return event;
}

gpointer
ufo_ir_op_set_generate_kernel(UfoResources *resources) {
    return kernel_from_name(resources, "operation_set");
}

gpointer
ufo_ir_op_inv (UfoBuffer *arg,
               gpointer   command_queue,
               gpointer   kernel) {
    UfoRequisition requisition;
    ufo_buffer_get_requisition (arg, &requisition);

    cl_mem d_arg = ufo_buffer_get_device_image (arg, command_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg(kernel, 0, sizeof(void *), (void *) &d_arg));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg(kernel, 1, sizeof(void *), (void *) &d_arg));
    cl_event event;
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel(command_queue, kernel,
                                                      requisition.n_dims, NULL, requisition.dims,
                                                      NULL, 0, NULL, &event));

    return event;
}

gpointer
ufo_ir_op_inv_generate_kernel(UfoResources *resources) {
    return kernel_from_name(resources, "operation_inv");
}

gpointer ufo_ir_op_mul (UfoBuffer *arg1,
                        UfoBuffer *arg2,
                        UfoBuffer *out,
                        gpointer   command_queue,
                        gpointer   kernel) {
    return operation (arg1, arg2, out, command_queue, kernel);
}

gpointer ufo_ir_op_mul_generate_kernel(UfoResources *resources){
    return kernel_from_name(resources, "operation_mul");
}

gpointer ufo_ir_op_add (UfoBuffer *arg1,
                        UfoBuffer *arg2,
                        UfoBuffer *out,
                        gpointer   command_queue,
                        gpointer   kernel)
{
    return operation (arg1, arg2, out, command_queue, kernel);
}

gpointer
ufo_ir_op_add_generate_kernel(UfoResources *resources){
    return kernel_from_name(resources, "operation_add");
}

gpointer ufo_ir_op_mul_rows (UfoBuffer *arg1,
                             UfoBuffer *arg2,
                             UfoBuffer *out,
                             guint offset,
                             guint n,
                             gpointer command_queue,
                             gpointer kernel) {
    UfoRequisition arg1_requisition, arg2_requisition, out_requisition;
    ufo_buffer_get_requisition (arg1, &arg1_requisition);
    ufo_buffer_get_requisition (arg2, &arg2_requisition);
    ufo_buffer_get_requisition (out, &out_requisition);

    if (arg1_requisition.dims[0] != arg2_requisition.dims[0] ||
        arg1_requisition.dims[0] != out_requisition.dims[0]) {
        g_error ("Number of columns is different.");
        return NULL;
    }

    if (arg1_requisition.dims[1] < offset + n ||
        arg2_requisition.dims[1] < offset + n ||
        out_requisition.dims[1] < offset + n) {
        g_error ("Rows are not enough.");
        return NULL;
    }

    cl_mem d_arg1 = ufo_buffer_get_device_image (arg1, command_queue);
    cl_mem d_arg2 = ufo_buffer_get_device_image (arg2, command_queue);
    cl_mem d_out  = ufo_buffer_get_device_image (out, command_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_arg1));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(void *), (void *) &d_arg2));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof(void *), (void *) &d_out));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 3, sizeof(unsigned int), (void *) &offset));

    UfoRequisition operation_requisition = out_requisition;
    operation_requisition.dims[1] = n;

    cl_event event;
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       operation_requisition.n_dims, NULL, operation_requisition.dims,
                                                       NULL, 0, NULL, &event));

    return event;
}

gpointer ufo_ir_op_mul_rows_generate_kernel(UfoResources *resources) {
    return kernel_from_name(resources, "op_mulRows");
}

