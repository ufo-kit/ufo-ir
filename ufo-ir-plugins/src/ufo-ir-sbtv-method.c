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
#include <stdio.h>

#define KERNELS_FILE_NAME "sb-gradient.cl"
#define MU 5E-01
#define LAMBDA 1E-01
#define EPS 2.2204E-16

typedef struct _FilterWorkSet FilterWorkSet;

struct _FilterWorkSet
{
    UfoTask *fft_task;
    UfoTask *filter_task;
    UfoTask *ifft_task;

    UfoBuffer *fft_result;
    UfoBuffer *filter_result;
    UfoBuffer *ifft_result;

    UfoRequisition fft_result_req;
    UfoRequisition filter_result_req;
    UfoRequisition ifft_result_req;
};

static void ufo_method_interface_init (UfoMethodIface *iface);
static void ufo_copyable_interface_init (UfoCopyableIface *iface);
static gboolean ufo_ir_sbtv_method_process_real (UfoMethod *method, UfoBuffer *input, UfoBuffer *output, gpointer  pevent);

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
static FilterWorkSet CreateFilterWorkSet(UfoBuffer *input, UfoIrMethod *method);
static void Filter(UfoBuffer *buffer, FilterWorkSet *work_set, gpointer *cmd_queue);
static void DebugWrite(UfoBuffer *buffer, const char *file_name);
static void update_db(UfoBuffer *u, UfoBuffer *dx, UfoBuffer *dy, UfoBuffer *bx, UfoBuffer *by, UfoMethod *method, gpointer cmd_queue, UfoResources *resources);
static void calculate_b(UfoBuffer *fbp, UfoBuffer *dx, UfoBuffer *dy, UfoBuffer *bx, UfoBuffer *by, UfoBuffer *b, UfoMethod *method, gpointer cmd_queue, UfoResources *resources);
static void twoAraysIterator(UfoBuffer *arg1, UfoBuffer *arg2, UfoBuffer *output, gpointer command_queue, void (*operation)(const float *, const float *, float *));
static void normalize(UfoBuffer *to_norm, gfloat mult, gpointer cmd_queue);

static void cgs(UfoBuffer *b, UfoBuffer *x, UfoBuffer *x0, guint maxIter, UfoBuffer *sino,
                UfoIrProjector *projector, UfoIrProjectionsSubset *subsets, guint subsetsCnt, UfoMethod *method,
                UfoResources *resources, gpointer *cmd_queue, FilterWorkSet *filter_work_set);

static void processA(UfoMethod *method, UfoBuffer *in, UfoBuffer *out, UfoBuffer *sino,
                     UfoIrProjector *projector, UfoIrProjectionsSubset *subsets, guint subsetsCnt,
                     UfoResources *resources, gpointer *cmd_queue, FilterWorkSet *filter_work_set);

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


static gboolean
ufo_ir_sbtv_method_process_real (UfoMethod *method,
                                 UfoBuffer *input,
                                 UfoBuffer *output,
                                 gpointer  pevent)
{
    UfoRequisition sinogramReq;
    ufo_buffer_get_requisition(input, &sinogramReq);

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

    FilterWorkSet filter_work_set = CreateFilterWorkSet(input, UFO_IR_METHOD(method));

    UfoIrGeometry *geometry = NULL;
    g_object_get (projector, "geometry", &geometry, NULL);

    guint n_subsets = 0;
    UfoIrProjectionsSubset *subsets = generate_subsets (geometry, &n_subsets);

    UfoBuffer *f = ufo_buffer_dup(input);
    ufo_buffer_copy(input, f);
    normalize(f, 1.0f, cmd_queue);
    Filter(f, &filter_work_set, cmd_queue);

    // precompute At(f)
    UfoBuffer *fbp = ufo_buffer_dup(output);
    ufo_op_set(fbp, 0.0f, resources, cmd_queue);
    for (guint i = 0 ; i < n_subsets; i++)
    {
        ufo_ir_projector_BP (projector, fbp, f, &subsets[i], 1.0f,NULL);
    }

    // u = At(f)
    UfoBuffer *u = output;
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

    // fbp = fbp * mu
    mult(fbp, MU, cmd_queue);

    // Main loop
    for(guint iterationNum = 0; iterationNum < max_iterations; ++iterationNum)
    {
        g_print("SBTV iteration: %d\n", iterationNum);
        ufo_buffer_copy(u, up);

        calculate_b(fbp, dx, dy, bx, by, b, method, cmd_queue, resources);

        cgs(b, u, up, 30, f, projector, subsets, n_subsets, method, resources, cmd_queue, &filter_work_set);

        char str[40];
        sprintf(&str, "debug/out_%d.tiff", (int)iterationNum);

        DebugWrite(up, &str);


        update_db(u, dx, dy, bx, by, method, cmd_queue, resources);

    }

    return TRUE;
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
                UfoMethod *method, UfoResources *resources, gpointer *cmd_queue, FilterWorkSet *filter_work_set)
{
//    DebugWrite(b,"debug/cgs/b.tiff");
//    DebugWrite(x,"debug/cgs/x.tiff");
//    DebugWrite(x0,"debug/cgs/x0.tiff");
    gfloat tol = 1E-06;

    gfloat n2b = l2_norm(b, cmd_queue);

    ufo_buffer_copy(x0, x);

    guint flag = 1;

    UfoBuffer *xmin;
    xmin = ufo_buffer_dup(x);
    ufo_buffer_copy(x, xmin);

    gfloat tolb = tol * n2b;

    // r = b - A * x
    UfoBuffer *r = ufo_buffer_dup(b);

    processA(method, x, r, sino, projector, subsets, subsetsCnt, resources, cmd_queue, filter_work_set);

    ufo_op_deduction(b, r, r, resources,cmd_queue);
//    DebugWrite(r, "debug/cgs/r.tiff");

    gfloat normr = l2_norm(r, cmd_queue);

    if(normr <= tolb)
    {
        // Initial guess is good enough
        g_print("Initial guess is good enough");
        return;
    }

    UfoBuffer *rt = ufo_buffer_dup(r);
    ufo_buffer_copy(r, rt);

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

        processA(method, ph, vh, sino, projector, subsets, subsetsCnt, resources, cmd_queue, filter_work_set);
//        DebugWrite(vh, "debug/cgs/vh.tiff");
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
//        DebugWrite(x, "debug/cgs/afterItX.tiff");
        processA(method, uh, qh, sino, projector, subsets, subsetsCnt, resources, cmd_queue, filter_work_set);

        ufo_op_add2(r, qh, -alpha, r, resources, cmd_queue);
        normr = l2_norm(r, cmd_queue);
//        DebugWrite(r, "debug/cgs/afterItR.tiff");
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
                     UfoResources *resources, gpointer *cmd_queue, FilterWorkSet *filter_work_set)
{
//    DebugWrite(in, "debug/processA.tiff");
    ufo_op_set(out, 0.0f, resources, cmd_queue);
    // mu * At(A(z))
    UfoBuffer *tempA = ufo_buffer_dup(sino);
    ufo_op_set(tempA, 0.0f, resources, cmd_queue);

    UfoBuffer *tempAt = ufo_buffer_dup(in);
    ufo_op_set(tempAt, 0.0f, resources, cmd_queue);

    for (guint i = 0 ; i < subsetsCnt; i++)
    {
        ufo_ir_projector_FP (projector, in, tempA, &subsets[i], 1.0f,NULL);
    }
    Filter(tempA, filter_work_set, cmd_queue);
    for (guint i = 0 ; i < subsetsCnt; i++)
    {
        ufo_ir_projector_BP (projector, tempAt, tempA, &subsets[i], 1.0f,NULL);
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

//    DebugWrite(tempAt, "debug/tempAt.tiff");
//    DebugWrite(out, "debug/inGrad.tiff");
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
    ufo_buffer_get_requisition (arg2, &arg2_requisition);

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
        //return;
    }

    length = length1;

    if(length1 != length2)
    {
        g_print("Buffers are not equal\n");
    }

    gfloat *outputs = ufo_buffer_get_host_array (output, command_queue);
    for(guint i = 0; i < length; i++)
    {
        operation(&values1[i], &values2[i], &outputs[i]);
    }
}

static FilterWorkSet CreateFilterWorkSet(UfoBuffer *input, UfoIrMethod *method)
{
    FilterWorkSet work_set;
    UfoPluginManager *manager = ufo_plugin_manager_new();
    UfoResources *resources = NULL;
    g_object_get (method,
                  "ufo-resources",     &resources,
//                  "command-queue",     &cmd_queue,
//                  "projection-model",  &projector,
//                  "max-iterations",    &max_iterations,
                  NULL);
    gpointer context = ufo_resources_get_context(resources);

    UfoNode *proc_node  = ufo_ir_method_get_proc_node(UFO_IR_METHOD(method));
    UfoBuffer *inputs[1]; // temp arrey for invocing get_requisition method
    GError *error = NULL;

    // Creating FFT task
    work_set.fft_task = UFO_TASK(ufo_plugin_manager_get_task (manager, "fft", NULL));
    ufo_task_node_set_proc_node(UFO_TASK_NODE(work_set.fft_task), UFO_NODE(proc_node));
    g_object_set(work_set.fft_task, "dimensions", 1, NULL);
    ufo_task_setup(work_set.fft_task, resources, &error);
    if (error)
    {
        g_printerr("\nError: SBTV fft setup: %s\n", error->message);
    }

    inputs[0] = input;
    ufo_task_get_requisition(work_set.fft_task, inputs, &work_set.fft_result_req); // Get requisition of fft output
    work_set.fft_result = ufo_buffer_new(&work_set.fft_result_req, context); // Creating temp buffer for FFT result

    // Creating filter task
    work_set.filter_task = UFO_TASK(ufo_plugin_manager_get_task (manager, "filter", NULL));
    ufo_task_node_set_proc_node(UFO_TASK_NODE(work_set.filter_task), proc_node);
    ufo_task_setup(work_set.filter_task, resources, &error);
    if (error)
    {
        g_printerr("\nError: SBTV filter setup: %s\n", error->message);
    }
    inputs[0] = work_set.fft_result;
    ufo_task_get_requisition(work_set.filter_task, inputs, &work_set.filter_result_req); // Requisition for filter result
    work_set.filter_result = ufo_buffer_new(&work_set.filter_result_req, context); // temp array for filter result

    // Creating IFFT
    work_set.ifft_task = UFO_TASK(ufo_plugin_manager_get_task (manager, "ifft", NULL));
    ufo_task_node_set_proc_node(UFO_TASK_NODE(work_set.ifft_task), proc_node);
    g_object_set(work_set.ifft_task, "dimensions", 1, NULL);
    ufo_task_setup(work_set.ifft_task, resources, &error);
    if (error)
    {
        g_printerr("\nError: SBTV ifft setup: %s\n", error->message);
    }
    inputs[0] = work_set.filter_result;
    ufo_task_get_requisition(work_set.ifft_task, inputs, &work_set.ifft_result_req); // Get requisition of fft output
    work_set.ifft_result = ufo_buffer_new(&work_set.ifft_result_req, context); // Creating temp buffer for FFT result
    // --Load and setup UFO tasks--

    g_object_unref(manager);
    g_object_unref(resources);
    return work_set;
}

static void Filter(UfoBuffer *buffer, FilterWorkSet *work_set, gpointer *cmd_queue)
{
    UfoBuffer *inputs[1];

    inputs[0] = buffer;
    ufo_task_process(work_set->fft_task, inputs, work_set->fft_result, &work_set->fft_result_req);

    inputs[0] = work_set->fft_result;
    ufo_task_process(work_set->filter_task, inputs, work_set->filter_result, &work_set->filter_result_req);

    inputs[0] = work_set->filter_result;
    ufo_task_process(work_set->ifft_task, inputs, work_set->ifft_result, &work_set->ifft_result_req);

    // Cut buffer
    gfloat *ifft_result_array = ufo_buffer_get_host_array (work_set->ifft_result, cmd_queue);
    gfloat *buffer_array = ufo_buffer_get_host_array (buffer, cmd_queue);

    UfoRequisition buffer_requisition;
    ufo_buffer_get_requisition (buffer, &buffer_requisition);

    guint height = buffer_requisition.dims[1];
    guint width = buffer_requisition.dims[0];
    guint big_width = work_set->ifft_result_req.dims[0];

    for(guint i = 0; i < height; i++)
    {
        for(guint j = 0; j < width; j++)
        {
            //buffer_array[i * width + j] = ifft_result_array[i * big_width + j];
        }
    }
}

static void DebugWrite(UfoBuffer *buffer, const char *file_name)
{
    //return;
    UfoPluginManager *manager = ufo_plugin_manager_new();
    UfoTask *writer = UFO_TASK(ufo_plugin_manager_get_task (manager, "writer", NULL));
    //ufo_task_node_set_proc_node(UFO_TASK_NODE(writer), proc_node);
    g_object_set (G_OBJECT (writer), "filename", file_name, NULL);
    GError *error = NULL;
    ufo_task_setup(writer, NULL, &error);
    if (error)
    {
        g_printerr("\nError: SBTV ifft setup: %s\n", error->message);
        return;
    }

    UfoBuffer *inputs[1]; // temp arrey for invocing get_requisition method
    inputs[0] = buffer;
    ufo_task_process(writer, inputs, NULL, NULL);

    g_object_unref(manager);
    g_object_unref(writer);
}

static void update_db(UfoBuffer *u, UfoBuffer *dx, UfoBuffer *dy, UfoBuffer *bx, UfoBuffer *by, UfoMethod *method, gpointer cmd_queue, UfoResources *resources)
{
    // Mem allocation
    UfoBuffer *tmpx = ufo_buffer_dup(u);
    UfoBuffer *tmpy = ufo_buffer_dup(u);
    UfoBuffer *s = ufo_buffer_dup(u);
    UfoBuffer *temp_pow = ufo_buffer_dup(u);
    UfoBuffer *tresh = ufo_buffer_dup(u);
    UfoBuffer *temp_s_top = ufo_buffer_dup(u);
    UfoBuffer *Z = ufo_buffer_dup(u);
    ufo_op_set(Z, 0.0f, resources, cmd_queue);
    UfoBuffer *e12 = ufo_buffer_dup(u);
    ufo_op_set(e12, 1E-12, resources, cmd_queue);

    gfloat dLambda = - 1 / LAMBDA;

    // tmpx = Dx(u)+bx;
    dx_op(method, u, tmpx, cmd_queue);
    ufo_op_add(tmpx, bx, tmpx, resources, cmd_queue);

    // tmpy = Dy(u)+by;
    dy_op(method, u, tmpy, cmd_queue);
    ufo_op_add(tmpy, by, tmpy, resources, cmd_queue);

    // s = sqrt((tmpx.^2 + tmpy.^2));
    elementsMult(tmpx, tmpx, temp_pow, cmd_queue);
    ufo_buffer_copy(temp_pow, s);
    elementsMult(tmpy, tmpy, temp_pow, cmd_queue);
    ufo_op_add(s, temp_pow, s, resources, cmd_queue);
    matrixSqrt(s, cmd_queue);

    // tresh = max(s-1/lambda,Z)./max(1e-12,s);
    ufo_op_set(temp_s_top, dLambda, resources, cmd_queue);
    ufo_op_add(temp_s_top, s, temp_s_top, resources, cmd_queue);
    elementsMax(temp_s_top, Z, temp_s_top, cmd_queue);
    elementsMax(s, e12, e12, cmd_queue);
    elementsDiv(temp_s_top, e12, tresh, cmd_queue);

    // dx = tresh.*tmpx;
    elementsMult(tresh, tmpx, dx, cmd_queue);

    // dy = tresh.*tmpy;
    elementsMult(tresh, tmpy, dy, cmd_queue);

    // bx = tmpx-dx;
    ufo_op_deduction(tmpx, dx, bx, resources, cmd_queue);

    // by = tmpy-dy;
    ufo_op_deduction(tmpy, dy, by, resources, cmd_queue);

    // release memory
    g_object_unref(tmpx);
    g_object_unref(tmpy);
    g_object_unref(s);
    g_object_unref(temp_pow);
    g_object_unref(tresh);
    g_object_unref(temp_s_top);
    g_object_unref(Z);
    g_object_unref(e12);
}

static void calculate_b(UfoBuffer *fbp, UfoBuffer *dx, UfoBuffer *dy, UfoBuffer *bx, UfoBuffer *by, UfoBuffer *b, UfoMethod *method, gpointer cmd_queue, UfoResources *resources)
{
    UfoBuffer *tmpx = ufo_buffer_dup(fbp);
    UfoBuffer *tmpy = ufo_buffer_dup(fbp);
    UfoBuffer *tmpDif = ufo_buffer_dup(fbp);

    // tmpx = DXT(dx - bx);
    ufo_op_deduction(dx, bx, tmpDif, resources, cmd_queue);
    dxt_op(method, tmpDif, tmpx, cmd_queue);

    // tmpx = DYT(dy - by);
    ufo_op_deduction(dy, by, tmpDif, resources, cmd_queue);
    dyt_op(method, tmpDif, tmpy, cmd_queue);

    // b = mu * At(f) + lambda * (tmpx + tmpy)
    ufo_op_add(tmpx, tmpy, b, resources, cmd_queue);
    mult(b, LAMBDA, cmd_queue);
    ufo_op_add(fbp, b, b, resources, cmd_queue);

    g_object_unref(tmpx);
    g_object_unref(tmpy);
    g_object_unref(tmpDif);
}

static void normalize(UfoBuffer *to_norm, gfloat mult, gpointer cmd_queue)
{
    UfoRequisition requisition;
    ufo_buffer_get_requisition (to_norm, &requisition);

    gfloat *values = ufo_buffer_get_host_array (to_norm, cmd_queue);

    guint length = 1;
    for(guint dimension = 0; dimension < requisition.n_dims; dimension++)
    {
        length *= (guint)requisition.dims[dimension];
    }

    gfloat max = -1000.f;
    gfloat min =  1000.f;

    for(guint i = 0; i < length; i++)
    {
        max = fmax(values[i],max);
        min = fmin(values[i],min);
    }

    gfloat delta = 1 / (max - min);


    for(guint i = 0; i < length; i++)
    {
        values[i] = delta * values[i] - min * delta;
    }
}
