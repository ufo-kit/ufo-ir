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

#include "ufo-ir-sbtv-task.h"
#include "core/ufo-ir-method-task.h"
#include "core/ufo-ir-gradient-processor.h"
#include "core/ufo-ir-basic-ops-processor.h"
#include "core/ufo-ir-projector-task.h"

#define EPS 2.2204E-16

static void ufo_ir_sbtv_task_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ufo_ir_sbtv_task_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ufo_ir_sbtv_task_setup (UfoTask *task, UfoResources *resources, GError **error);
static void ufo_ir_sbtv_task_finalize (GObject *object);
static gboolean ufo_ir_sbtv_task_process (UfoTask *task, UfoBuffer **inputs, UfoBuffer *output, UfoRequisition *requisition);
static const gchar *ufo_ir_sbtv_task_get_package_name(UfoTaskNode *self);

static void calculate_b(UfoIrSbtvTask *self, UfoBuffer *fbp, UfoBuffer *dx, UfoBuffer *dy, UfoBuffer *bx, UfoBuffer *by, UfoBuffer *b);
static void update_db(UfoIrSbtvTask *self, UfoBuffer *u, UfoBuffer *dx, UfoBuffer *dy, UfoBuffer *bx, UfoBuffer *by);
static void cgs(UfoIrSbtvTask *self, UfoBuffer *b, UfoBuffer *x, UfoBuffer *x0, guint maxIter, UfoBuffer *sino);
static void processA(UfoIrSbtvTask *self, UfoBuffer *in, UfoBuffer *out, UfoBuffer *sino);

struct _UfoIrSbtvTaskPrivate {
    // Method parameters
    gfloat mu;
    gfloat lambda;

    UfoIrGradientProcessor *gradient_processor;
    UfoIrBasicOpsProcessor *bo_processor;
};

static void ufo_task_interface_init (UfoTaskIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrSbtvTask, ufo_ir_sbtv_task, UFO_TYPE_TASK_NODE,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_IR_SBTV_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_SBTV_TASK, UfoIrSbtvTaskPrivate))

enum {
    PROP_0,
    PROP_MU,
    PROP_LAMBDA,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

// -----------------------------------------------------------------------------
// Init methods
// -----------------------------------------------------------------------------
static void
ufo_task_interface_init (UfoTaskIface *iface)
{
    iface->setup = ufo_ir_sbtv_task_setup;
    iface->process = ufo_ir_sbtv_task_process;
}

static void
ufo_ir_sbtv_task_class_init (UfoIrSbtvTaskClass *klass)
{
    GObjectClass *oclass = G_OBJECT_CLASS (klass);

    oclass->set_property = ufo_ir_sbtv_task_set_property;
    oclass->get_property = ufo_ir_sbtv_task_get_property;
    oclass->finalize = ufo_ir_sbtv_task_finalize;

    UfoTaskNodeClass * tnclass= UFO_TASK_NODE_CLASS(klass);
    tnclass->get_package_name = ufo_ir_sbtv_task_get_package_name;

    properties[PROP_LAMBDA] =
            g_param_spec_float("lambda",
                               "Lambda",
                               "Lambda",
                               0.0f, G_MAXFLOAT, 0.1f,
                               G_PARAM_READWRITE);
    properties[PROP_LAMBDA] =
            g_param_spec_float("mu",
                               "Mu",
                               "Mu",
                               0.0f, G_MAXFLOAT, 0.5f,
                               G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (oclass, i, properties[i]);

    g_type_class_add_private (oclass, sizeof(UfoIrSbtvTaskPrivate));
}

static void
ufo_ir_sbtv_task_init(UfoIrSbtvTask *self)
{
    self->priv = UFO_IR_SBTV_TASK_GET_PRIVATE(self);
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Getters and setters
// -----------------------------------------------------------------------------
gfloat ufo_ir_sbtv_task_get_mu(UfoIrSbtvTask *self) {
    UfoIrSbtvTaskPrivate *priv = UFO_IR_SBTV_TASK_GET_PRIVATE (self);
    return priv->mu;
}

void   ufo_ir_sbtv_task_set_mu(UfoIrSbtvTask *self, gfloat value) {
    UfoIrSbtvTaskPrivate *priv = UFO_IR_SBTV_TASK_GET_PRIVATE (self);
    priv->mu = value;
}

gfloat ufo_ir_sbtv_task_get_lambda(UfoIrSbtvTask *self) {
    UfoIrSbtvTaskPrivate *priv = UFO_IR_SBTV_TASK_GET_PRIVATE (self);
    return priv->lambda;
}

void   ufo_ir_sbtv_task_set_lambda(UfoIrSbtvTask *self, gfloat value) {
    UfoIrSbtvTaskPrivate *priv = UFO_IR_SBTV_TASK_GET_PRIVATE (self);
    priv->lambda = value;
}

static void
ufo_ir_sbtv_task_set_property (GObject *object,
                                       guint property_id,
                                       const GValue *value,
                                       GParamSpec *pspec) {
    UfoIrSbtvTask *self = UFO_IR_SBTV_TASK (object);

    switch (property_id) {
        case PROP_MU:
            ufo_ir_sbtv_task_set_mu(self, g_value_get_float(value));
            break;
        case PROP_LAMBDA:
            ufo_ir_sbtv_task_set_lambda(self, g_value_get_float(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_sbtv_task_get_property (GObject *object,
                                       guint property_id,
                                       GValue *value,
                                       GParamSpec *pspec) {
    UfoIrSbtvTask *self = UFO_IR_SBTV_TASK (object);

    switch (property_id) {
        case PROP_MU:
            g_value_set_float(value, ufo_ir_sbtv_task_get_mu(self));
            break;
        case PROP_LAMBDA:
            g_value_set_float(value, ufo_ir_sbtv_task_get_lambda(self));
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
ufo_ir_sbtv_task_new (void)
{
    return UFO_NODE (g_object_new (UFO_IR_TYPE_SBTV_TASK, NULL));
}

static void
ufo_ir_sbtv_task_setup (UfoTask *task,
                        UfoResources *resources,
                        GError **error)
{
    UfoIrSbtvTaskPrivate *priv = UFO_IR_SBTV_TASK_GET_PRIVATE (task);
    UfoGpuNode *node = UFO_GPU_NODE (ufo_task_node_get_proc_node (UFO_TASK_NODE(task)));
    cl_command_queue cmd_queue = (cl_command_queue)ufo_gpu_node_get_cmd_queue (node);
    priv->gradient_processor = ufo_ir_gradient_processor_new(resources, cmd_queue);
    priv->bo_processor = ufo_ir_basic_ops_processor_new(resources, cmd_queue);
}

static gboolean
ufo_ir_sbtv_task_process (UfoTask *task,
                         UfoBuffer **inputs,
                         UfoBuffer *output,
                         UfoRequisition *requisition)
{
    UfoIrSbtvTask *self = UFO_IR_SBTV_TASK(task);
    UfoIrSbtvTaskPrivate *priv = UFO_IR_SBTV_TASK_GET_PRIVATE (self);
    UfoBuffer *input = inputs[0];
    UfoRequisition sinogramReq;
    ufo_buffer_get_requisition(input, &sinogramReq);

    // UfoResources   *resources = NULL;
    // gpointer       *cmd_queue = NULL;

    UfoIrProjectorTask *projector = ufo_ir_method_task_get_projector(UFO_IR_METHOD_TASK(self));
    guint          max_iterations = ufo_ir_method_task_get_iterations_number(UFO_IR_METHOD_TASK(self));

    UfoBuffer *f = ufo_buffer_dup(input);
    ufo_buffer_copy(input, f);
    ufo_ir_basic_ops_processor_normalization(priv->bo_processor, f);

    // precompute At(f)
    UfoBuffer *fbp = ufo_buffer_dup(output);
    ufo_ir_basic_ops_processor_set(priv->bo_processor, fbp, 0.0f);
    ufo_ir_projector_task_set_relaxation(projector, 1.0f);
    ufo_ir_state_dependent_task_backward(UFO_IR_STATE_DEPENDENT_TASK(projector), &fbp, f, NULL);

    // u = At(f)
    UfoBuffer *u = output;
    ufo_buffer_copy(fbp, u);

    UfoBuffer *up = ufo_buffer_dup(fbp);

    UfoBuffer *Z = ufo_buffer_dup(fbp);
    ufo_ir_basic_ops_processor_set(priv->bo_processor, Z, 0.0f);

    UfoBuffer *b = ufo_buffer_dup(fbp);

    UfoBuffer *bx = ufo_buffer_dup(fbp);
    ufo_ir_basic_ops_processor_set(priv->bo_processor, bx, 0.0f);

    UfoBuffer *by = ufo_buffer_dup(fbp);
    ufo_ir_basic_ops_processor_set(priv->bo_processor, by, 0.0f);

    UfoBuffer *dx = ufo_buffer_dup(fbp);
    ufo_ir_basic_ops_processor_set(priv->bo_processor, dx, 0.0f);

    UfoBuffer *dy = ufo_buffer_dup(fbp);
    ufo_ir_basic_ops_processor_set(priv->bo_processor, dy, 0.0f);

    // fbp = fbp * mu
    ufo_ir_basic_ops_processor_mul_scalar(priv->bo_processor, fbp, priv->mu);

    // Main loop
    for(guint iterationNum = 0; iterationNum < max_iterations; ++iterationNum)
    {
        g_print("SBTV iteration: %d\n", iterationNum);
        ufo_buffer_copy(u, up);

        calculate_b(self, fbp, dx, dy, bx, by, b);

        cgs(self, b, u, up, 30, f);

        //char str[40];
        //sprintf(&str, "debug/out_%d.tiff", (int)iterationNum);

        //DebugWrite(up, &str);

        update_db(self, u, dx, dy, bx, by);

    }

    return TRUE;
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// GObject methods
// -----------------------------------------------------------------------------
static void
ufo_ir_sbtv_task_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_ir_sbtv_task_parent_class)->finalize (object);
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// TaskNode methods
// -----------------------------------------------------------------------------
static const gchar *ufo_ir_sbtv_task_get_package_name(UfoTaskNode *self) {
    return g_strdup("ir");
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------------
static void
calculate_b(UfoIrSbtvTask *self, UfoBuffer *fbp, UfoBuffer *dx, UfoBuffer *dy, UfoBuffer *bx, UfoBuffer *by, UfoBuffer *b)
{
    UfoIrSbtvTaskPrivate *priv = UFO_IR_SBTV_TASK_GET_PRIVATE (self);
    UfoBuffer *tmpx = ufo_buffer_dup(fbp);
    UfoBuffer *tmpy = ufo_buffer_dup(fbp);
    UfoBuffer *tmpDif = ufo_buffer_dup(fbp);

    // tmpx = DXT(dx - bx);
    ufo_ir_basic_ops_processor_deduction(priv->bo_processor, dx, bx, tmpDif);
    ufo_ir_gradient_processor_dxt_op(priv->gradient_processor, tmpDif, tmpx);

    // tmpx = DYT(dy - by);
    ufo_ir_basic_ops_processor_deduction(priv->bo_processor, dy, by, tmpDif);
    ufo_ir_gradient_processor_dyt_op(priv->gradient_processor, tmpDif, tmpy);

    // b = mu * At(f) + lambda * (tmpx + tmpy)
    ufo_ir_basic_ops_processor_add(priv->bo_processor, tmpx, tmpy, b);
    ufo_ir_basic_ops_processor_mul_scalar(priv->bo_processor, b, priv->lambda);
    ufo_ir_basic_ops_processor_add(priv->bo_processor, fbp, b, b);

    g_object_unref(tmpx);
    g_object_unref(tmpy);
    g_object_unref(tmpDif);
}

static void
update_db(UfoIrSbtvTask *self, UfoBuffer *u, UfoBuffer *dx, UfoBuffer *dy, UfoBuffer *bx, UfoBuffer *by)
{
    UfoIrSbtvTaskPrivate *priv = UFO_IR_SBTV_TASK_GET_PRIVATE (self);
    // Mem allocation
    UfoBuffer *tmpx = ufo_buffer_dup(u);
    UfoBuffer *tmpy = ufo_buffer_dup(u);
    UfoBuffer *s = ufo_buffer_dup(u);
    UfoBuffer *temp_pow = ufo_buffer_dup(u);
    UfoBuffer *tresh = ufo_buffer_dup(u);
    UfoBuffer *temp_s_top = ufo_buffer_dup(u);
    UfoBuffer *Z = ufo_buffer_dup(u);
    ufo_ir_basic_ops_processor_set(priv->bo_processor, Z, 0.0f);
    UfoBuffer *e12 = ufo_buffer_dup(u);
    ufo_ir_basic_ops_processor_set(priv->bo_processor, e12, 1E-12);

    gfloat dLambda = - 1 / priv->lambda;

    // tmpx = Dx(u)+bx;
    ufo_ir_gradient_processor_dx_op(priv->gradient_processor, u, tmpx);
    ufo_ir_basic_ops_processor_add(priv->bo_processor, tmpx, bx, tmpx);

    // tmpy = Dy(u)+by;
    ufo_ir_gradient_processor_dy_op(priv->gradient_processor, u, tmpy);
    ufo_ir_basic_ops_processor_add(priv->bo_processor, tmpy, by, tmpy);

    // s = sqrt((tmpx.^2 + tmpy.^2));
    ufo_ir_basic_ops_processor_mul_element_wise(priv->bo_processor, tmpx, tmpx, temp_pow);
    ufo_buffer_copy(temp_pow, s);
    ufo_ir_basic_ops_processor_mul_element_wise(priv->bo_processor, tmpy, tmpy, temp_pow);
    ufo_ir_basic_ops_processor_add(priv->bo_processor, s, temp_pow, s);
    ufo_ir_basic_ops_processor_sqrt(priv->bo_processor, s);

    // tresh = max(s-1/lambda,Z)./max(1e-12,s);
    ufo_ir_basic_ops_processor_set(priv->bo_processor, temp_s_top, dLambda);
    ufo_ir_basic_ops_processor_add(priv->bo_processor, temp_s_top, s, temp_s_top);
    ufo_ir_basic_ops_processor_max_element_wise(priv->bo_processor, temp_s_top, Z, temp_s_top);
    ufo_ir_basic_ops_processor_max_element_wise(priv->bo_processor, s, e12, e12);
    ufo_ir_basic_ops_processor_div_element_wise(priv->bo_processor, temp_s_top, e12, tresh);

    // dx = tresh.*tmpx;
    ufo_ir_basic_ops_processor_mul_element_wise(priv->bo_processor, tresh, tmpx, dx);

    // dy = tresh.*tmpy;
    ufo_ir_basic_ops_processor_mul_element_wise(priv->bo_processor, tresh, tmpy, dy);

    // bx = tmpx-dx;
    ufo_ir_basic_ops_processor_deduction(priv->bo_processor, tmpx, dx, bx);

    // by = tmpy-dy;
    ufo_ir_basic_ops_processor_deduction(priv->bo_processor, tmpy, dy, by);

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

static void
cgs(UfoIrSbtvTask *self, UfoBuffer *b, UfoBuffer *x, UfoBuffer *x0, guint maxIter, UfoBuffer *sino)
{
    UfoIrSbtvTaskPrivate *priv = UFO_IR_SBTV_TASK_GET_PRIVATE (self);
//    DebugWrite(b,"debug/cgs/b.tiff");
//    DebugWrite(x,"debug/cgs/x.tiff");
//    DebugWrite(x0,"debug/cgs/x0.tiff");
    gfloat tol = 1E-06;

    gfloat n2b = ufo_ir_basic_ops_processor_l2_norm(priv->bo_processor, b);

    ufo_buffer_copy(x0, x);

    guint flag = 1;

    UfoBuffer *xmin;
    xmin = ufo_buffer_dup(x);
    ufo_buffer_copy(x, xmin);

    gfloat tolb = tol * n2b;

    // r = b - A * x
    UfoBuffer *r = ufo_buffer_dup(b);

    processA(self, x, r, sino);

    ufo_ir_basic_ops_processor_deduction(priv->bo_processor, b, r, r);
//    DebugWrite(r, "debug/cgs/r.tiff");

    gfloat normr = ufo_ir_basic_ops_processor_l2_norm(priv->bo_processor, r);

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
    ufo_ir_basic_ops_processor_set(priv->bo_processor, u, 0.0f);

    UfoBuffer *p = ufo_buffer_dup(r);
    ufo_ir_basic_ops_processor_set(priv->bo_processor, p, 0.0f);
    UfoBuffer *ph = ufo_buffer_dup(r);

    UfoBuffer *q = ufo_buffer_dup(r);
    ufo_ir_basic_ops_processor_set(priv->bo_processor, q, 0.0f);

    UfoBuffer *vh = ufo_buffer_dup(r);

    UfoBuffer *tempSum = ufo_buffer_dup(r);
    ufo_ir_basic_ops_processor_set(priv->bo_processor, tempSum, 0.0f);

    UfoBuffer *uh = ufo_buffer_dup(u);
    UfoBuffer *qh = ufo_buffer_dup(u);
    guint maxstagsteps = 3;
    guint iterationNum;

    for(iterationNum = 0; iterationNum < maxIter; ++iterationNum)
    {
        gfloat rho1 = rho;
        rho = ufo_ir_basic_ops_processor_dot_product(priv->bo_processor, rt, r);
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
            ufo_ir_basic_ops_processor_add2(priv->bo_processor, r, q, beta, u);

            ufo_ir_basic_ops_processor_add2(priv->bo_processor, q, p, beta, tempSum);
            ufo_ir_basic_ops_processor_add2(priv->bo_processor, u, tempSum, beta, p);
        }

        ufo_buffer_copy(p, ph);

        processA(self, ph, vh, sino);
//        DebugWrite(vh, "debug/cgs/vh.tiff");

        gfloat rtvh = ufo_ir_basic_ops_processor_dot_product(priv->bo_processor, rt, vh);
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

        ufo_ir_basic_ops_processor_add2(priv->bo_processor, u, vh, -alpha, q);
        ufo_ir_basic_ops_processor_add(priv->bo_processor, u, q, uh);

        // Check for stagnation
        if(fabs(alpha) * ufo_ir_basic_ops_processor_l2_norm(priv->bo_processor, uh) < EPS * ufo_ir_basic_ops_processor_l2_norm(priv->bo_processor, x))
        {
            stag += 1;
        }
        else
        {
            stag = 0;
        }

        ufo_ir_basic_ops_processor_add2(priv->bo_processor, x, uh, alpha, x);
//        DebugWrite(x, "debug/cgs/afterItX.tiff");
        processA(self, uh, qh, sino);

        ufo_ir_basic_ops_processor_add2(priv->bo_processor, r, qh, -alpha, r);
        normr = ufo_ir_basic_ops_processor_l2_norm(priv->bo_processor, r);
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

static void
processA(UfoIrSbtvTask *self, UfoBuffer *in, UfoBuffer *out, UfoBuffer *sino)
{
    UfoIrSbtvTaskPrivate *priv = UFO_IR_SBTV_TASK_GET_PRIVATE (self);
    UfoIrStateDependentTask *projector = UFO_IR_STATE_DEPENDENT_TASK(ufo_ir_method_task_get_projector(UFO_IR_METHOD_TASK(self)));
//    DebugWrite(in, "debug/processA.tiff");

    ufo_ir_basic_ops_processor_set(priv->bo_processor, out, 0.0f);
    // mu * At(A(z))
    UfoBuffer *tempA = ufo_buffer_dup(sino);
    ufo_ir_basic_ops_processor_set(priv->bo_processor, tempA, 0.0f);

    UfoBuffer *tempAt = ufo_buffer_dup(in);
    ufo_ir_basic_ops_processor_set(priv->bo_processor, tempAt, 0.0f);

    ufo_ir_state_dependent_task_forward(projector, &in, tempA, NULL);
    ufo_ir_state_dependent_task_backward(projector, &tempAt, tempA, NULL);

    ufo_ir_basic_ops_processor_mul_scalar(priv->bo_processor, tempAt, priv->mu);
    // DYT(DY(z))
    UfoBuffer *tempD = ufo_buffer_dup(in);
    ufo_ir_gradient_processor_dy_op(priv->gradient_processor, in, tempD);
    ufo_ir_gradient_processor_dyt_op(priv->gradient_processor, tempD, out);

    // DXT(DX(z))
    ufo_ir_gradient_processor_dx_op(priv->gradient_processor, in, tempD);
    UfoBuffer *tempDxt = ufo_buffer_dup(in);
    ufo_ir_gradient_processor_dxt_op(priv->gradient_processor, tempD, tempDxt);
    ufo_ir_basic_ops_processor_add(priv->bo_processor ,out, tempDxt, out); // DYT + DXT

    ufo_ir_basic_ops_processor_mul_scalar(priv->bo_processor, out, priv->lambda);

//    DebugWrite(tempAt, "debug/tempAt.tiff");
//    DebugWrite(out, "debug/inGrad.tiff");
    ufo_ir_basic_ops_processor_add(priv->bo_processor, out, tempAt, out);

    g_object_unref(tempA);
    g_object_unref(tempAt);
    g_object_unref(tempD);
    g_object_unref(tempDxt);
}
// -----------------------------------------------------------------------------
