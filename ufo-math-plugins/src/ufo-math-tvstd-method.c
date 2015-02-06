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

#include "ufo-math-tvstd-method.h"
#include <ufo/ufo.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

static void ufo_method_interface_init (UfoMethodIface *iface);
static void ufo_copyable_interface_init (UfoCopyableIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoMathTvStdMethod, ufo_math_tvstd_method, UFO_TYPE_PROCESSOR,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_METHOD,
                                                ufo_method_interface_init)
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_COPYABLE,
                                                ufo_copyable_interface_init))

#define UFO_MATH_TVSTD_METHOD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_MATH_TYPE_TVSTD_METHOD, UfoMathTvStdMethodPrivate))

GQuark
ufo_math_tvstd_method_error_quark (void)
{
    return g_quark_from_static_string ("ufo-math-tvstd-method-error-quark");
}

struct _UfoMathTvStdMethodPrivate {
    guint       n_iters;
    gfloat      relaxation_factor;
    UfoBuffer   *grad;
    gpointer    kernel;
};

enum {
    PROP_0 = 0,
    PROP_NUM_ITERATIONS,
    PROP_RELAXATION_FACTOR,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoMethod *
ufo_math_tvstd_method_new (void)
{
    return UFO_METHOD (g_object_new (UFO_MATH_TYPE_TVSTD_METHOD, NULL));
}

static void
ufo_math_tvstd_method_setup_real (UfoProcessor  *processor,
                                  UfoResources  *resources,
                                  GError        **error)
{
    UFO_PROCESSOR_CLASS (ufo_math_tvstd_method_parent_class)->setup (processor,
                                                                     resources,
                                                                     error);
    UfoMathTvStdMethodPrivate *priv =
        UFO_MATH_TVSTD_METHOD_GET_PRIVATE (processor);

    priv->kernel = ufo_resources_get_kernel (resources,
                                             "ufo-math-tvstd-method.cl",
                                             "l1_grad",
                                             error);
}

static void
ufo_math_tvstd_method_init (UfoMathTvStdMethod *self)
{
    UfoMathTvStdMethodPrivate *priv = NULL;
    self->priv = priv = UFO_MATH_TVSTD_METHOD_GET_PRIVATE (self);
    priv->relaxation_factor = 0.5f;
    priv->n_iters = 20;
    priv->grad = NULL;
    priv->kernel = NULL;
}

static void
ufo_math_tvstd_method_set_property (GObject      *object,
                                        guint        property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
    UfoMathTvStdMethodPrivate *priv =
        UFO_MATH_TVSTD_METHOD_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_NUM_ITERATIONS:
            priv->n_iters = g_value_get_uint (value);
            break;
        case PROP_RELAXATION_FACTOR:
            priv->relaxation_factor = g_value_get_float (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_math_tvstd_method_get_property (GObject    *object,
                                    guint      property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
    UfoMathTvStdMethodPrivate *priv =
        UFO_MATH_TVSTD_METHOD_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_NUM_ITERATIONS:
            g_value_set_uint (value, priv->n_iters);
            break;
        case PROP_RELAXATION_FACTOR:
            g_value_set_float (value, priv->relaxation_factor);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_math_tvstd_method_dispose (GObject *object)
{
    UfoMathTvStdMethodPrivate *priv =
        UFO_MATH_TVSTD_METHOD_GET_PRIVATE (object);
    UFO_RESOURCES_CHECK_CLERR (clReleaseKernel (priv->kernel));
    G_OBJECT_CLASS (ufo_math_tvstd_method_parent_class)->dispose (object);
}

static gboolean
ufo_math_tvstd_method_process_real (UfoMethod *method,
                                    UfoBuffer *input,
                                    UfoBuffer *output,
                                    gpointer  pevent)
{
    UfoMathTvStdMethodPrivate *priv = UFO_MATH_TVSTD_METHOD_GET_PRIVATE (method);
    gpointer     kernel = priv->kernel;

    UfoResources *resources = ufo_processor_get_resources (UFO_PROCESSOR (method));
    gpointer cmd_queue = ufo_processor_get_command_queue (UFO_PROCESSOR (method));
    gpointer profiler  = ufo_processor_get_profiler (UFO_PROCESSOR (method)); 
    if (priv->grad) {
        UfoRequisition requisition;
        ufo_buffer_get_requisition (input, &requisition);
        ufo_buffer_resize (priv->grad, &requisition);
    }
    else {
        priv->grad = ufo_buffer_dup (input);
    }
    // is it really need?
    ufo_op_set (priv->grad, 0, resources, cmd_queue);

    UfoRequisition input_req;
    ufo_buffer_get_requisition (input, &input_req);

    cl_mem d_input = ufo_buffer_get_device_image (input, cmd_queue);
    cl_mem d_grad = ufo_buffer_get_device_image (priv->grad, cmd_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(cl_mem), &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(cl_mem), &d_grad));

    gfloat factor = 0.0f, l1 = 0.0f;
    guint iteration = 0;

    while (iteration < priv->n_iters) {
        ufo_profiler_call (profiler, cmd_queue, kernel, input_req.n_dims,
                           input_req.dims, NULL);

        l1 = ufo_op_l1_norm (priv->grad, resources, cmd_queue);
        factor = priv->relaxation_factor / l1;
        ufo_op_deduction2 (input, priv->grad, factor, output, resources, cmd_queue);
        iteration++;
    }

    return TRUE;
}

static void
ufo_method_interface_init (UfoMethodIface *iface)
{
    iface->process = ufo_math_tvstd_method_process_real;
}

static UfoCopyable *
ufo_math_tvstd_method_copy_real (gpointer origin,
                                 gpointer _copy)
{
    UfoCopyable *copy;
    if (_copy)
        copy = UFO_COPYABLE(_copy);
    else
        copy = UFO_COPYABLE (ufo_math_tvstd_method_new());

    UfoMathTvStdMethodPrivate *priv =
        UFO_MATH_TVSTD_METHOD_GET_PRIVATE (origin);

    //
    // copy other parameters
    g_object_set (G_OBJECT(copy), "relaxation-factor", priv->relaxation_factor, NULL);
    g_object_set (G_OBJECT(copy), "num-iterations", priv->n_iters, NULL);

    return copy;
}

static void
ufo_copyable_interface_init (UfoCopyableIface *iface)
{
    iface->copy = ufo_math_tvstd_method_copy_real;
}

static void
ufo_math_tvstd_method_class_init (UfoMathTvStdMethodClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = ufo_math_tvstd_method_dispose;
    gobject_class->set_property = ufo_math_tvstd_method_set_property;
    gobject_class->get_property = ufo_math_tvstd_method_get_property;

    properties[PROP_RELAXATION_FACTOR] =
        g_param_spec_float("relaxation-factor",
                           "The relaxation parameter.",
                           "The relaxation parameter.",
                           0.0f, G_MAXFLOAT, 0.5f,
                           G_PARAM_READWRITE);

    properties[PROP_NUM_ITERATIONS] =
        g_param_spec_uint ("num-iterations",
                           "Number of iterations",
                           "Number of iterations",
                           0, 1000, 20,
                           G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof(UfoMathTvStdMethodPrivate));

    UFO_PROCESSOR_CLASS (klass)->setup  = ufo_math_tvstd_method_setup_real;
}
