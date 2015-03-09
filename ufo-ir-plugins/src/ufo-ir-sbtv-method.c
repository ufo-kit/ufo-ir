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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "ufo-ir-sbtv-method.h"
#include <math.h>
#include <ufo/ufo.h>

#define KERNELS_FILE_NAME "sb-gradient.cl"
#define MU 5E-01
#define LAMBDA 1E-01


static void ufo_method_interface_init (UfoMethodIface *iface);
static void ufo_copyable_interface_init (UfoCopyableIface *iface);

static void ufo_ir_sbtv_method_mult(UfoBuffer *buffer, gfloat mult, gpointer command_queue);
static gpointer ufo_ir_sbtv_method_dx  (UfoMethod *method, UfoBuffer *input, UfoBuffer *output, gpointer command_queue);
static gpointer ufo_ir_sbtv_method_dxt (UfoMethod *method, UfoBuffer *input, UfoBuffer *output, gpointer command_queue);
static gpointer ufo_ir_sbtv_method_dy  (UfoMethod *method, UfoBuffer *input, UfoBuffer *output, gpointer command_queue);
static gpointer ufo_ir_sbtv_method_dyt (UfoMethod *method, UfoBuffer *input, UfoBuffer *output, gpointer command_queue);


G_DEFINE_TYPE_WITH_CODE (UfoIrSbtvMethod, ufo_ir_sbtv_method, UFO_IR_TYPE_METHOD,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_METHOD,
                                                ufo_method_interface_init)
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_COPYABLE,
                                                ufo_copyable_interface_init))

#define UFO_IR_SBTV_METHOD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_SBTV_METHOD, UfoIrSbtvMethodPrivate))

GQuark
ufo_ir_sbtv_method_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-sbtv-method-error-quark");
}

struct _UfoIrSbtvMethodPrivate
{
    gpointer dxKernel;
    gpointer dxtKernel;
    gpointer dyKernel;
    gpointer dytKernel;
};

static gfloat
l2_norm2(UfoBuffer *arg,
        UfoResources *resources,
        gpointer command_queue)
{
    UfoRequisition arg_requisition;
    ufo_buffer_get_requisition (arg, &arg_requisition);

    gfloat *values = ufo_buffer_get_host_array (arg, command_queue);

    guint length = 1;
    for(guint dimension = 0; dimension < arg_requisition.n_dims; dimension++)
    {
        length *= (guint)arg_requisition.dims[dimension];
    }

    guint partsCnt = (guint)arg_requisition.dims[arg_requisition.n_dims -1];
    guint partLen = length / partsCnt;

    gfloat norm = 0;
    for(guint partNum = 0; partNum < partsCnt; partNum++)
    {
        gfloat partNorm = 0;
        for(guint i = 0; i < partLen; i++)
        {
            guint index = partNum * i;
            partNorm += values[index] * values[index];
        }
        norm += partNorm;
    }

    return norm;
}

static UfoIrProjectionsSubset *
generate_subsets (UfoIrGeometry *geometry, guint *n_subsets)
{
    gfloat *sin_values = ufo_ir_geometry_scan_angles_host (geometry, SIN_VALUES);
    gfloat *cos_values = ufo_ir_geometry_scan_angles_host (geometry, COS_VALUES);
    guint n_angles = 0;
    g_object_get (geometry, "num-angles", &n_angles, NULL);

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
ufo_ir_sbtv_method_setup_real (UfoProcessor *processor,
                               UfoResources *resources,
                               GError       **error)
{
    UFO_PROCESSOR_CLASS (ufo_ir_sbtv_method_parent_class)->setup (processor, resources, error);
    if (error && *error)
    {
        return;
    }

    UfoIrSbtvMethodPrivate *priv = UFO_IR_SBTV_METHOD_GET_PRIVATE(processor);

    priv->dxKernel = ufo_resources_get_kernel(resources, KERNELS_FILE_NAME,"Dx", error);
    if(*error)
    {
        return;
    }
    priv->dyKernel = ufo_resources_get_kernel(resources, KERNELS_FILE_NAME,"Dy", error);
    if(*error)
    {
        return;
    }
    priv->dxtKernel = ufo_resources_get_kernel(resources, KERNELS_FILE_NAME,"Dxt", error);
    if(*error)
    {
        return;
    }
    priv->dytKernel = ufo_resources_get_kernel(resources, KERNELS_FILE_NAME,"Dyt", error);
    if(*error)
    {
        return;
    }

}

UfoIrMethod *
ufo_ir_sbtv_method_new (void)
{
    return UFO_IR_METHOD (g_object_new (UFO_IR_TYPE_SBTV_METHOD, NULL));
}

static void
ufo_ir_sbtv_method_init (UfoIrSbtvMethod *self)
{
}

static gboolean
ufo_ir_sbtv_method_process_real (UfoMethod *method,
                                 UfoBuffer *input,
                                 UfoBuffer *output,
                                 gpointer  pevent)
{
    UfoResources   *resources = NULL;
    UfoIrProjector *projector = NULL;
    gpointer       *cmd_queue = NULL;

    guint          max_iterations = 0;
    g_object_get (method,
                  "ufo-resources",     &resources,
                  "command-queue",     &cmd_queue,
                  "projection-model",  &projector,
                  "max-iterations",    &max_iterations,
                  NULL);

    UfoIrGeometry *geometry = NULL;
    g_object_get (projector, "geometry", &geometry, NULL);

    guint n_subsets = 0;
    UfoIrProjectionsSubset *subsets = generate_subsets (geometry, &n_subsets);

    // Rename
    UfoBuffer *f = input;

    // precompute At(f)
    UfoBuffer *fbp = ufo_buffer_dup(output);
    ufo_op_set(fbp, 0.0f, resources, cmd_queue);
    for (guint i = 0 ; i < n_subsets; i++)
    {
        ufo_ir_projector_BP (projector, fbp, f, &subsets[i], 1.0f,NULL);
    }

    // u = At(f)
    UfoBuffer *u = ufo_buffer_dup(fbp);
    ufo_buffer_copy(fbp, u);

    UfoBuffer *Z = ufo_buffer_dup(fbp);
    ufo_op_set(Z, 0.0f, resources, cmd_queue);

    UfoBuffer *b = ufo_buffer_dup(fbp);

    UfoBuffer *bx = ufo_buffer_dup(fbp);
    ufo_op_set(bx, 0.0f, resources, cmd_queue);

    UfoBuffer *by = ufo_buffer_dup(fbp);
    ufo_op_set(by, 0.0f, resources, cmd_queue);

    UfoBuffer *dx = ufo_buffer_dup(fbp);
    ufo_op_set(dx, 0.0f, resources, cmd_queue);

    UfoBuffer *dy = ufo_buffer_dup(fbp);
    ufo_op_set(dy, 0.0f, resources, cmd_queue);

    UfoBuffer *tmpf = ufo_buffer_dup(fbp);
    ufo_op_set(tmpf, 0.0f, resources, cmd_queue);

    UfoBuffer *tmpx = ufo_buffer_dup(fbp);
    UfoBuffer *tmpy = ufo_buffer_dup(fbp);

    UfoBuffer *tmpDifx = ufo_buffer_dup(fbp);
    UfoBuffer *tmpDify = ufo_buffer_dup(fbp);

    UfoBuffer *tmpw = ufo_buffer_dup(fbp);
    ufo_op_set(tmpw, 0.0f, resources, cmd_queue);


    // fbp = fbp * mu
    ufo_ir_sbtv_method_mult(fbp, MU, cmd_queue);

    // Main loop
    for(guint iterationNum = 0; iterationNum < max_iterations; ++iterationNum)
    {
        // up = u;
        //ufo_buffer_copy(u, up);

        // tmpx = DXT(dx - bx);
        ufo_op_add2(dx, bx, -1.0f, tmpDifx, resources, cmd_queue);
        ufo_ir_sbtv_method_dxt(method, tmpDifx, tmpx, cmd_queue);

        // tmpx = DYT(dy - by);
        ufo_op_add2(dy, by, -1.0f, tmpDify, resources, cmd_queue);
        ufo_ir_sbtv_method_dyt(method, tmpDify, tmpy, cmd_queue);

        // b = mu * At(f) + lambda * (tmpx + tmpy)
        ufo_op_add(tmpDifx, tmpDify, b, resources, cmd_queue);
        ufo_ir_sbtv_method_mult(b, LAMBDA, cmd_queue);
        ufo_op_add(fbp, b, b, resources, cmd_queue);




    }

    return TRUE;
}



static gboolean
ufo_ir_sbtv_method_cgls (UfoMethod *method,
                         UfoBuffer *b,
                         UfoBuffer *initialGuess,
                         UfoBuffer *x,
                         UfoResources   *resources,
                         UfoIrProjector *projector,
                         UfoIrProjectionsSubset *subsets,
                         guint n_subsets,
                         gpointer *cmd_queue,
                         guint max_iterations)
{

    gfloat shift = 0;

    ufo_buffer_copy(initialGuess, x);

    // r = b - A * x
    UfoBuffer *r = ufo_buffer_dup(b);
    ufo_buffer_copy(b, r);
    for (guint i = 0 ; i < n_subsets; i++)
    {
        ufo_ir_projector_FP (projector, x, r, &subsets[i], -1.0f,NULL);
    }

    // s = A' * r - shift * x
    UfoBuffer *s = ufo_buffer_dup(x);
    UfoBuffer *tempBp = ufo_buffer_dup(x); // tempBp will be used in main loop too
    ufo_op_set(tempBp, 0.0f, resources, cmd_queue);
    ufo_op_set(s, 0.0f, resources, cmd_queue);
    for (guint i = 0 ; i < n_subsets; i++)
    {
        ufo_ir_projector_BP (projector, tempBp, r, &subsets[i], 1.0f, NULL);
    }
    ufo_op_add2(tempBp, x, -1.0f * shift, s, resources, cmd_queue);

    // Initialize

    // p = s
    UfoBuffer * p = ufo_buffer_dup(s);
    ufo_buffer_copy(s, p);

    // norms0 = norm(s)
    // gamma = norms0^2
    gfloat gamma = l2_norm2(s, resources, cmd_queue);

    // Main loop
    for(guint iterationNum = 0; iterationNum < max_iterations; ++iterationNum)
    {
        // q = A * p
        UfoBuffer *q = ufo_buffer_dup(b);
        ufo_op_set(q, 0.0f, resources, cmd_queue);
        for (guint i = 0 ; i < n_subsets; i++)
        {
            ufo_ir_projector_FP (projector, p, q, &subsets[i], 1.0f,NULL);
        }

        // delta = norm(q)^2 + shift * norm(p) ^ 2
        gfloat delta = l2_norm2(q, resources, cmd_queue) + shift * l2_norm2(p, resources, cmd_queue);
        if(delta == 0)
        {
            delta = 1E-06;
        }

        // alpha = gamma / delta
        gfloat alpha = gamma / delta;

        // x = x + alpha * p;
        ufo_op_add2(x, p, alpha, x, resources, cmd_queue);

        // r = r - alpha * q
        ufo_op_add2(r, q, -1.0f * alpha, r, resources, cmd_queue);

        // s = A' * r - shift * x;
        ufo_op_set(tempBp, 0.0f, resources, cmd_queue);
        ufo_op_set(s, 0.0f, resources, cmd_queue);
        for (guint i = 0 ; i < n_subsets; i++)
        {
            ufo_ir_projector_BP (projector, tempBp, r, &subsets[i],1.0f,NULL);
        }
        ufo_op_add2(tempBp, x, -1.0f * shift, s, resources, cmd_queue);

        gfloat beta = 1.0f / gamma;
        gamma = l2_norm2(s, resources, cmd_queue);
        beta *= gamma;
        ufo_op_add2(s, p, beta, p, resources, cmd_queue);
    }

    return TRUE;
}

static void
ufo_method_interface_init (UfoMethodIface *iface)
{
    iface->process = ufo_ir_sbtv_method_process_real;
}

static UfoCopyable *
ufo_ir_sbtv_method_copy_real (gpointer origin,
                              gpointer _copy)
{
    UfoCopyable *copy;
    if (_copy)
        copy = UFO_COPYABLE(_copy);
    else
        copy = UFO_COPYABLE (ufo_ir_sbtv_method_new());

    return copy;
}

static void
ufo_copyable_interface_init (UfoCopyableIface *iface)
{
    iface->copy = ufo_ir_sbtv_method_copy_real;
}

static void
ufo_ir_sbtv_method_class_init (UfoIrSbtvMethodClass *klass)
{
    UFO_PROCESSOR_CLASS (klass)->setup = ufo_ir_sbtv_method_setup_real;
}



static gpointer
ufo_ir_sbtv_method_dx (UfoMethod *method,
                       UfoBuffer *input,
                       UfoBuffer *output,
                       gpointer command_queue)
{
    UfoRequisition requisition;
    cl_event event;

    UfoIrSbtvMethodPrivate *priv = UFO_IR_SBTV_METHOD_GET_PRIVATE(method);
    cl_kernel kernel = priv->dxKernel;

    ufo_buffer_get_requisition (input, &requisition);
    cl_mem d_input = ufo_buffer_get_device_array(input, command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, command_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       requisition.n_dims, NULL, requisition.dims,
                                                       NULL, 0, NULL, &event));

    return event;
}

static gpointer
ufo_ir_sbtv_method_dxt (UfoMethod *method,
                       UfoBuffer *input,
                       UfoBuffer *output,
                       gpointer command_queue)
{
    UfoRequisition requisition;
    cl_event event;

    UfoIrSbtvMethodPrivate *priv = UFO_IR_SBTV_METHOD_GET_PRIVATE(method);
    cl_kernel kernel = priv->dxKernel;

    ufo_buffer_get_requisition (input, &requisition);
    cl_mem d_input = ufo_buffer_get_device_array(input, command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, command_queue);

    gint stopIndex = requisition.dims[0] - 1;

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(gint), (void *) &stopIndex));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       requisition.n_dims, NULL, requisition.dims,
                                                       NULL, 0, NULL, &event));

    return event;
}

static gpointer
ufo_ir_sbtv_method_dy (UfoMethod *method,
                       UfoBuffer *input,
                       UfoBuffer *output,
                       gpointer command_queue)
{
    UfoRequisition requisition;
    cl_event event;

    UfoIrSbtvMethodPrivate *priv = UFO_IR_SBTV_METHOD_GET_PRIVATE(method);
    cl_kernel kernel = priv->dxKernel;

    ufo_buffer_get_requisition (input, &requisition);
    cl_mem d_input = ufo_buffer_get_device_array(input, command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, command_queue);

    gint lastOffset = requisition.dims[0] * requisition.dims[1];

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(gint), (void *) &lastOffset));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       requisition.n_dims, NULL, requisition.dims,
                                                       NULL, 0, NULL, &event));

    return event;
}

static gpointer
ufo_ir_sbtv_method_dyt (UfoMethod *method,
                        UfoBuffer *input,
                        UfoBuffer *output,
                        gpointer command_queue)
{
    UfoRequisition requisition;
    cl_event event;

    UfoIrSbtvMethodPrivate *priv = UFO_IR_SBTV_METHOD_GET_PRIVATE(method);
    cl_kernel kernel = priv->dxKernel;

    ufo_buffer_get_requisition (input, &requisition);
    cl_mem d_input = ufo_buffer_get_device_array(input, command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, command_queue);

    gint lastOffset = requisition.dims[0] * requisition.dims[1];
    gint stopIndex = requisition.dims[1] - 1;

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(gint), (void *) &lastOffset));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(gint), (void *) &stopIndex));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       requisition.n_dims, NULL, requisition.dims,
                                                       NULL, 0, NULL, &event));

    return event;
}

static void ufo_ir_sbtv_method_mult(UfoBuffer *buffer, gfloat mult, gpointer command_queue)
{
    UfoRequisition arg_requisition;
    ufo_buffer_get_requisition (buffer, &arg_requisition);

    gfloat *values = ufo_buffer_get_host_array (buffer, command_queue);

    guint length = 1;
    for(guint dimension = 0; dimension < arg_requisition.n_dims; dimension++)
    {
        length *= (guint)arg_requisition.dims[dimension];
    }

    for(guint i = 0; i < length; i++)
    {
        values[i] *= mult;
    }
}

