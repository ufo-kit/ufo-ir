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
#define EPS 2.2204E-16

static void ufo_method_interface_init (UfoMethodIface *iface);
static void ufo_copyable_interface_init (UfoCopyableIface *iface);

static void mult(UfoBuffer *buffer, gfloat mult, gpointer command_queue);
static gfloat l2_norm(UfoBuffer *arg, gpointer command_queue);
static gfloat dotProduct(UfoBuffer *arg1, UfoBuffer *arg2, gpointer command_queue);
static void elementsMult(UfoBuffer *arg1, UfoBuffer *arg2, UfoBuffer *output, gpointer command_queue);
static void elementsDiv(UfoBuffer *arg1, UfoBuffer *arg2, UfoBuffer *output, gpointer command_queue);
static void elementsMax(UfoBuffer *arg1, UfoBuffer *arg2, UfoBuffer *output, gpointer command_queue);
static void matrixSqrt(UfoBuffer *arg, gpointer command_queue);
static gpointer dx_op  (UfoMethod *method, UfoBuffer *input, UfoBuffer *output, gpointer command_queue);
static gpointer dxt_op (UfoMethod *method, UfoBuffer *input, UfoBuffer *output, gpointer command_queue);
static gpointer dy_op  (UfoMethod *method, UfoBuffer *input, UfoBuffer *output, gpointer command_queue);
static gpointer dyt_op (UfoMethod *method, UfoBuffer *input, UfoBuffer *output, gpointer command_queue);

static void twoAraysIterator(UfoBuffer *arg1, UfoBuffer *arg2, UfoBuffer *output, gpointer command_queue, void (*operation)(const float *, const float *, float *));

static void cgs(UfoBuffer *b, UfoBuffer *x, UfoBuffer *x0, guint maxIter, UfoBuffer *sino,
                UfoIrProjector *projector, UfoIrProjectionsSubset *subsets, guint subsetsCnt, UfoMethod *method,
                UfoResources *resources, gpointer *cmd_queue);

static void processA(UfoMethod *method, UfoBuffer *in, UfoBuffer *out, UfoBuffer *sino,
                     UfoIrProjector *projector, UfoIrProjectionsSubset *subsets, guint subsetsCnt,
                     UfoResources *resources, gpointer *cmd_queue);


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

    UfoBuffer *up = ufo_buffer_dup(fbp);

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

    UfoBuffer *tempDub = ufo_buffer_dup(fbp);
    UfoBuffer *s = ufo_buffer_dup(fbp);
    UfoBuffer *smone = ufo_buffer_dup(fbp);
    UfoBuffer *e12 = ufo_buffer_dup(fbp);
    ufo_op_set(e12, 1E-12, resources, cmd_queue);
    UfoBuffer *e12max = ufo_buffer_dup(fbp);
    UfoBuffer *tresh = ufo_buffer_dup(fbp);


    // fbp = fbp * mu
    mult(fbp, MU, cmd_queue);

    gfloat dLambda = 1 / LAMBDA;

    // Main loop
    for(guint iterationNum = 0; iterationNum < max_iterations; ++iterationNum)
    {
        g_print("SBTV iteration: %d\n", iterationNum);
        ufo_buffer_copy(u, up);

        // tmpx = DXT(dx - bx);
        ufo_op_add2(dx, bx, -1.0f, tmpDifx, resources, cmd_queue);
        dxt_op(method, tmpDifx, tmpx, cmd_queue);

        // tmpx = DYT(dy - by);
        ufo_op_add2(dy, by, -1.0f, tmpDify, resources, cmd_queue);
        dyt_op(method, tmpDify, tmpy, cmd_queue);

        // b = mu * At(f) + lambda * (tmpx + tmpy)
        ufo_op_add(tmpDifx, tmpDify, b, resources, cmd_queue);
        mult(b, LAMBDA, cmd_queue);
        ufo_op_add(fbp, b, b, resources, cmd_queue);

        cgs(b, u, up, 30, f, projector, subsets, n_subsets, method, resources, cmd_queue);

        dx_op(method, u, tmpx, cmd_queue);
        ufo_op_add(tmpx, bx, tmpx, resources, cmd_queue);
        elementsMult(tmpx, tmpx, tempDub, cmd_queue);

        ufo_buffer_copy(tempDub, s);

        dy_op(method, u, tmpy, cmd_queue);
        ufo_op_add(tmpy, by, tmpy, resources, cmd_queue);
        elementsMult(tmpy, tmpy, tempDub, cmd_queue);

        ufo_op_add(s, tempDub, s, resources, cmd_queue);
        matrixSqrt(s, cmd_queue);

        ufo_op_set(smone, -dLambda, resources, cmd_queue);
        ufo_op_add(smone, s, smone, resources, cmd_queue);

        elementsMax(smone, Z, smone, cmd_queue);
        elementsMax(s, e12, e12max, cmd_queue);

        elementsDiv(smone, e12max, tresh, cmd_queue);

        elementsMult(tresh, tmpx, dx, cmd_queue);
        elementsMult(tresh, tmpy, dy, cmd_queue);

        ufo_op_deduction(tmpx, dx, bx, resources, cmd_queue);
        ufo_op_deduction(tmpy, dy, by, resources, cmd_queue);

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
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    g_type_class_add_private (gobject_class, sizeof(UfoIrSbtvMethodPrivate));
}



static gpointer dx_op (UfoMethod *method, UfoBuffer *input, UfoBuffer *output, gpointer command_queue)
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

static gpointer dxt_op (UfoMethod *method, UfoBuffer *input, UfoBuffer *output, gpointer command_queue)
{
    UfoRequisition requisition;
    cl_event event;

    UfoIrSbtvMethodPrivate *priv = UFO_IR_SBTV_METHOD_GET_PRIVATE(method);
    cl_kernel kernel = priv->dxtKernel;

    ufo_buffer_get_requisition (input, &requisition);
    cl_mem d_input = ufo_buffer_get_device_array(input, command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, command_queue);

    int stopIndex = requisition.dims[0] - 1;

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(int), (void *) &stopIndex));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       requisition.n_dims, NULL, requisition.dims,
                                                       NULL, 0, NULL, &event));

    return event;
}

static gpointer dy_op (UfoMethod *method, UfoBuffer *input, UfoBuffer *output, gpointer command_queue)
{
    UfoRequisition requisition;
    cl_event event;

    UfoIrSbtvMethodPrivate *priv = UFO_IR_SBTV_METHOD_GET_PRIVATE(method);
    cl_kernel kernel = priv->dyKernel;

    ufo_buffer_get_requisition (input, &requisition);
    cl_mem d_input = ufo_buffer_get_device_array(input, command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, command_queue);

    int lastOffset = requisition.dims[0] * requisition.dims[1];

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(int), (void *) &lastOffset));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       requisition.n_dims, NULL, requisition.dims,
                                                       NULL, 0, NULL, &event));

    return event;
}

static gpointer dyt_op (UfoMethod *method, UfoBuffer *input, UfoBuffer *output, gpointer command_queue)
{
    UfoRequisition requisition;
    cl_event event;

    UfoIrSbtvMethodPrivate *priv = UFO_IR_SBTV_METHOD_GET_PRIVATE(method);
    cl_kernel kernel = priv->dytKernel;

    ufo_buffer_get_requisition (input, &requisition);
    cl_mem d_input = ufo_buffer_get_device_array(input, command_queue);
    cl_mem d_output = ufo_buffer_get_device_array(output, command_queue);

    gint lastOffset = requisition.dims[0] * requisition.dims[1];
    gint stopIndex = requisition.dims[1] - 1;

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(void *), (void *) &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(gint), (void *) &lastOffset));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof(gint), (void *) &stopIndex));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 3, sizeof(void *), (void *) &d_output));
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (command_queue, kernel,
                                                       requisition.n_dims, NULL, requisition.dims,
                                                       NULL, 0, NULL, &event));

    return event;
}

static void mult(UfoBuffer *buffer, gfloat mult, gpointer command_queue)
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

static void cgs(UfoBuffer *b, UfoBuffer *x, UfoBuffer *x0, guint maxIter, UfoBuffer *sino,
                UfoIrProjector *projector, UfoIrProjectionsSubset *subsets, guint subsetsCnt,
                UfoMethod *method, UfoResources *resources, gpointer *cmd_queue)
{
    gfloat tol = 1E-06;

    gfloat n2b = l2_norm(b, cmd_queue);

    ufo_buffer_copy(x0, x);

    guint flag = 1;

    UfoBuffer *xmin;
    xmin = ufo_buffer_dup(x);
    ufo_buffer_copy(x, xmin);

    guint imin = 0;
    gfloat tolb = tol * n2b;

    // r = b - A * x
    UfoBuffer *r = ufo_buffer_dup(b);
    ufo_buffer_copy(b, r);
    for (guint i = 0 ; i < subsetsCnt; i++)
    {
        ufo_ir_projector_FP (projector, x, r, &subsets[i], -1.0f,NULL);
    }

    gfloat normr = l2_norm(r, cmd_queue);
    gfloat normAct = normr;

    if(normr <= tolb)
    {
        // Initial guess is good enough
        g_print("Initial guess is good enough");
        return;
    }

    UfoBuffer *rt = ufo_buffer_dup(r);
    ufo_buffer_copy(r, rt);

    gfloat normMin = normr;

    gfloat rho = 1.0f;
    gfloat stag = 0.0f;

    UfoBuffer *u = ufo_buffer_dup(r);
    ufo_op_set(u, 0.0f, resources, cmd_queue);

    UfoBuffer *p = ufo_buffer_dup(r);
    ufo_op_set(p, 0.0f, resources, cmd_queue);
    UfoBuffer *ph = ufo_buffer_dup(r);

    UfoBuffer *q = ufo_buffer_dup(r);
    ufo_op_set(q, 0.0f, resources, cmd_queue);

    UfoBuffer *vh = ufo_buffer_dup(r);

    UfoBuffer *tempSum = ufo_buffer_dup(r);
    ufo_op_set(tempSum, 0.0f, resources, cmd_queue);

    UfoBuffer *uh = ufo_buffer_dup(u);
    UfoBuffer *qh = ufo_buffer_dup(u);
    guint maxstagsteps = 3;
    guint iterationNum;
    for(iterationNum = 0; iterationNum < maxIter; ++iterationNum)
    {
        gfloat rho1 = rho;
        rho = dotProduct(rt, r, cmd_queue);
        if(rho == 0 || isinf(rho) || isnan(rho))
        {
            flag = 4;
            break;
        }

        if(iterationNum == 0)
        {
            ufo_buffer_copy(r, u);
            ufo_buffer_copy(u, p);
        }
        else
        {
            gfloat beta = rho / rho1;
            if(beta == 0 || isinf(beta) || isnan(beta))
            {
                flag = 4;
                break;
            }
            ufo_op_add2(r, q, beta, u, resources, cmd_queue);

            ufo_op_add2(q, p, beta, tempSum, resources, cmd_queue);
            ufo_op_add2(u, tempSum, beta, p, resources, cmd_queue);
        }

        ufo_buffer_copy(p, ph);

        processA(method, ph, vh, sino, projector, subsets, subsetsCnt, resources, cmd_queue);

        gfloat rtvh = dotProduct(rt, vh, cmd_queue);
        gfloat alpha = 0;
        if(rtvh == 0)
        {
            flag = 4;
            break;
        }
        else
        {
            alpha = rho / rtvh;
        }

        if(isinf(alpha))
        {
            flag = 4;
            break;
        }

        ufo_op_add2(u, vh, -alpha, q, resources, cmd_queue);

        ufo_op_add(u, q, uh, resources, cmd_queue);

        // Check for stagnation
        if(fabs(alpha) * l2_norm(uh, cmd_queue) < EPS * l2_norm(x, cmd_queue))
        {
            stag += 1;
        }
        else
        {
            stag = 0;
        }

        ufo_op_add2(x, uh, alpha, x, resources, cmd_queue);

        processA(method, uh, qh, sino, projector, subsets, subsetsCnt, resources, cmd_queue);

        ufo_op_add2(r, qh, -alpha, r, resources, cmd_queue);
        normr = l2_norm(r, cmd_queue);

        if(normr <= tolb || stag >= maxstagsteps)
        {
            break;
        }
    }
    g_print("SGS %d iterations\n", iterationNum);

    g_object_unref(xmin);
    g_object_unref(r);
    g_object_unref(rt);
    g_object_unref(u);
    g_object_unref(p);
    g_object_unref(q);
    g_object_unref(vh);
    g_object_unref(tempSum);
    g_object_unref(uh);
    g_object_unref(qh);

}

static void processA(UfoMethod *method, UfoBuffer *in, UfoBuffer *out, UfoBuffer *sino,
                     UfoIrProjector *projector, UfoIrProjectionsSubset *subsets, guint subsetsCnt,
                     UfoResources *resources, gpointer *cmd_queue)
{
    ufo_op_set(out, 0.0f, resources, cmd_queue);

    // mu * At(A(z))
    UfoBuffer *tempA = ufo_buffer_dup(sino);
    ufo_op_set(tempA, 0.0f, resources, cmd_queue);

    UfoBuffer *tempAt = ufo_buffer_dup(in);
    ufo_op_set(tempAt, 0.0f, resources, cmd_queue);

    for (guint i = 0 ; i < subsetsCnt; i++)
    {
        ufo_ir_projector_FP (projector, in, tempA, &subsets[i], -1.0f,NULL);
    }
    for (guint i = 0 ; i < subsetsCnt; i++)
    {
        ufo_ir_projector_BP (projector, tempAt, tempA, &subsets[i], -1.0f,NULL);
    }

    mult(tempAt, MU, cmd_queue);

    // DYT(DY(z))
    UfoBuffer *tempD = ufo_buffer_dup(in);
    dy_op(method, in, tempD, cmd_queue);
    dyt_op(method, tempD, out, cmd_queue);

    // DXT(DX(z))
    dx_op(method, in, tempD, cmd_queue);
    UfoBuffer *tempDxt = ufo_buffer_dup(in);
    dxt_op(method, tempD, tempDxt, cmd_queue);

    ufo_op_add(out, tempDxt, out, resources, cmd_queue); // DYT + DXT
    mult(out, LAMBDA, cmd_queue);
    ufo_op_add(out, tempAt, out, resources, cmd_queue);

    g_object_unref(tempA);
    g_object_unref(tempAt);
    g_object_unref(tempD);
    g_object_unref(tempDxt);
}

static gfloat dotProduct(UfoBuffer *arg1, UfoBuffer *arg2, gpointer command_queue)
{
    UfoRequisition arg1_requisition;
    ufo_buffer_get_requisition (arg1, &arg1_requisition);

    gfloat *values1 = ufo_buffer_get_host_array (arg1, command_queue);

    guint length1 = 1;
    for(guint dimension = 0; dimension < arg1_requisition.n_dims; dimension++)
    {
        length1 *= (guint)arg1_requisition.dims[dimension];
    }

    UfoRequisition arg2_requisition;
    ufo_buffer_get_requisition (arg1, &arg2_requisition);

    gfloat *values2 = ufo_buffer_get_host_array (arg2, command_queue);

    guint length2 = 1;
    for(guint dimension = 0; dimension < arg2_requisition.n_dims; dimension++)
    {
        length2 *= (guint)arg2_requisition.dims[dimension];
    }

    guint length = 1;

    if(arg1_requisition.n_dims != arg2_requisition.n_dims)
    {
        g_print("Buffers are not equal\n");
        return -1.0f;
    }

    if(length1 == length2)
    {
        length = length1;
    }
    else
    {
        g_print("Buffers are not equal\n");
        return -1.0f;
    }

    guint partsCnt = (guint)arg1_requisition.dims[arg1_requisition.n_dims -1];
    guint partLen = length / partsCnt;

    gfloat norm = 0;
    for(guint partNum = 0; partNum < partsCnt; partNum++)
    {
        gfloat partNorm = 0;
        for(guint i = 0; i < partLen; i++)
        {
            guint index = partNum * i;
            partNorm += values1[index] * values2[index];
        }
        norm += partNorm;
    }

    return norm;
}

static gfloat l2_norm(UfoBuffer *arg, gpointer command_queue)
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

    norm = sqrt(norm);

    return norm;
}

static void elementsMultOperation(const float *in1,const float *in2, float *outVal)
{
    *outVal = (*in1) * (*in2);
}

static void elementsMult(UfoBuffer *arg1, UfoBuffer *arg2, UfoBuffer *output, gpointer command_queue)
{
    twoAraysIterator(arg1, arg2, output, command_queue, elementsMultOperation);
}

static void elementsDivOperation(const float *in1,const float *in2, float *outVal)
{
    *outVal = (*in1) / (*in2);
}

static void elementsDiv(UfoBuffer *arg1, UfoBuffer *arg2, UfoBuffer *output, gpointer command_queue)
{
    twoAraysIterator(arg1, arg2, output, command_queue, elementsDivOperation);
}

static void matrixSqrt(UfoBuffer *arg, gpointer command_queue)
{
    UfoRequisition arg_requisition;
    ufo_buffer_get_requisition (arg, &arg_requisition);

    gfloat *values = ufo_buffer_get_host_array (arg, command_queue);

    guint length = 1;
    for(guint dimension = 0; dimension < arg_requisition.n_dims; dimension++)
    {
        length *= (guint)arg_requisition.dims[dimension];
    }

    for(guint i = 0; i < length; i++)
    {
        values[i] =  sqrt(values[i]);
    }
}

static void elementsMaxOperation(const float *in1,const float *in2, float *outVal)
{
    *outVal = fmax(*in1,*in2);
}

static void elementsMax(UfoBuffer *arg1, UfoBuffer *arg2, UfoBuffer *output, gpointer command_queue)
{
    twoAraysIterator(arg1, arg2, output, command_queue, elementsMaxOperation);
}

static void twoAraysIterator(UfoBuffer *arg1, UfoBuffer *arg2, UfoBuffer *output, gpointer command_queue, void(*operation)(const float*, const float*, float*))
{
    UfoRequisition arg1_requisition;
    ufo_buffer_get_requisition (arg1, &arg1_requisition);

    gfloat *values1 = ufo_buffer_get_host_array (arg1, command_queue);

    guint length1 = 1;
    for(guint dimension = 0; dimension < arg1_requisition.n_dims; dimension++)
    {
        length1 *= (guint)arg1_requisition.dims[dimension];
    }

    UfoRequisition arg2_requisition;
    ufo_buffer_get_requisition (arg1, &arg2_requisition);

    gfloat *values2 = ufo_buffer_get_host_array (arg2, command_queue);

    guint length2 = 1;
    for(guint dimension = 0; dimension < arg2_requisition.n_dims; dimension++)
    {
        length2 *= (guint)arg2_requisition.dims[dimension];
    }

    guint length = 1;

    if(arg1_requisition.n_dims != arg2_requisition.n_dims)
    {
        g_print("Buffers are not equal\n");
        return;
    }

    if(length1 == length2)
    {
        length = length1;
    }
    else
    {
        g_print("Buffers are not equal\n");
        return;
    }

    gfloat *outputs = ufo_buffer_get_host_array (output, command_queue);
    for(guint i = 0; i < length; i++)
    {
        operation(&values1[i], &values2[i], &outputs[i]);
    }
}
