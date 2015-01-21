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

#include "ufo-math-shrinkage-method.h"
#include <ufo/ufo.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

static void ufo_method_interface_init (UfoMethodIface *iface);
static void ufo_copyable_interface_init (UfoCopyableIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoMathShrinkageMethod, ufo_math_shrinkage_method, UFO_TYPE_PROCESSOR,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_METHOD,
                                                ufo_method_interface_init)
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_COPYABLE,
                                                ufo_copyable_interface_init))

#define UFO_MATH_SHRINKAGE_METHOD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_MATH_TYPE_SHRINKAGE_METHOD, UfoMathShrinkageMethodPrivate))

GQuark
ufo_math_shrinkage_method_error_quark (void)
{
    return g_quark_from_static_string ("ufo-math-shrinkage-method-error-quark");
}

struct _UfoMathShrinkageMethodPrivate {
    float    shrinkage_param;
    gpointer kernel;
};

enum {
    PROP_0 = 0,
    PROP_SHRINKAGE_PARAMETER,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoMethod *
ufo_math_shrinkage_method_new (void)
{
    return UFO_METHOD (g_object_new (UFO_MATH_TYPE_SHRINKAGE_METHOD, NULL));
}

static void
ufo_math_shrinkage_method_setup_real (UfoProcessor  *processor,
                                      UfoResources  *resources,
                                      GError        **error)
{
    UFO_PROCESSOR_CLASS (ufo_math_shrinkage_method_parent_class)->setup (processor,
                                                                         resources,
                                                                         error);
    UfoMathShrinkageMethodPrivate *priv =
        UFO_MATH_SHRINKAGE_METHOD_GET_PRIVATE (processor);

    priv->kernel = ufo_resources_get_kernel (resources,
                                             "shrinkage.cl",
                                             "shrinkage",
                                             error);
}

static void
ufo_math_shrinkage_method_init (UfoMathShrinkageMethod *self)
{
    UfoMathShrinkageMethodPrivate *priv = NULL;
    self->priv = priv = UFO_MATH_SHRINKAGE_METHOD_GET_PRIVATE (self);
    priv->shrinkage_param = 1.0f;
}

static void
ufo_math_shrinkage_method_set_property (GObject      *object,
                                        guint        property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
    UfoMathShrinkageMethodPrivate *priv =
        UFO_MATH_SHRINKAGE_METHOD_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_SHRINKAGE_PARAMETER:
            priv->shrinkage_param = g_value_get_float (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_math_shrinkage_method_get_property (GObject    *object,
                                        guint      property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
    UfoMathShrinkageMethodPrivate *priv =
        UFO_MATH_SHRINKAGE_METHOD_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_SHRINKAGE_PARAMETER:
            g_value_set_float (value, priv->shrinkage_param);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_math_shrinkage_method_dispose (GObject *object)
{
    UfoMathShrinkageMethodPrivate *priv =
        UFO_MATH_SHRINKAGE_METHOD_GET_PRIVATE (object);
    UFO_RESOURCES_CHECK_CLERR (clReleaseKernel (priv->kernel));
    G_OBJECT_CLASS (ufo_math_shrinkage_method_parent_class)->dispose (object);
}

static gboolean
ufo_math_shrinkage_method_process_real (UfoMethod *method,
                                        UfoBuffer *input,
                                        UfoBuffer *output,
                                        gpointer  pevent)
{
    UfoMathShrinkageMethodPrivate *priv = UFO_MATH_SHRINKAGE_METHOD_GET_PRIVATE (method);
    cl_kernel kernel = priv->kernel;
    gfloat    param  = priv->shrinkage_param;
    cl_command_queue cmd_queue = NULL;
    UfoProfiler      *profiler = NULL;
    g_object_get (method,
                  "ufo-profiler", &profiler,
                  "command-queue", &cmd_queue,
                  NULL);

    cl_mem d_input = ufo_buffer_get_device_image (input, cmd_queue);
    cl_mem d_output = ufo_buffer_get_device_image (output, cmd_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof (cl_mem), &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof (cl_mem), &d_output));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof (gfloat), &param));

    UfoRequisition requisitions;
    ufo_buffer_get_requisition (input, &requisitions);

    ufo_profiler_call (profiler, cmd_queue, kernel, requisitions.n_dims,
                       requisitions.dims, NULL);

    return TRUE;
}

static void
ufo_method_interface_init (UfoMethodIface *iface)
{
    iface->process = ufo_math_shrinkage_method_process_real;
}

static UfoCopyable *
ufo_math_shrinkage_method_copy_real (gpointer origin,
                                     gpointer _copy)
{
    UfoCopyable *copy;
    if (_copy)
        copy = UFO_COPYABLE(_copy);
    else
        copy = UFO_COPYABLE (ufo_math_shrinkage_method_new());

    UfoMathShrinkageMethodPrivate *priv =
        UFO_MATH_SHRINKAGE_METHOD_GET_PRIVATE (origin);

    //
    // copy other parameters
    g_object_set (G_OBJECT(copy), "shrinkage-param", priv->shrinkage_param, NULL);

    return copy;
}

static void
ufo_copyable_interface_init (UfoCopyableIface *iface)
{
    iface->copy = ufo_math_shrinkage_method_copy_real;
}

static void
ufo_math_shrinkage_method_class_init (UfoMathShrinkageMethodClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = ufo_math_shrinkage_method_dispose;
    gobject_class->set_property = ufo_math_shrinkage_method_set_property;
    gobject_class->get_property = ufo_math_shrinkage_method_get_property;

    properties[PROP_SHRINKAGE_PARAMETER] =
        g_param_spec_float("shrinkage-parameter",
                           "Parameter of Shrinkage",
                           "Parameter of Shrinkage",
                           0.0f, G_MAXFLOAT, 1.0f,
                           G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof(UfoMathShrinkageMethodPrivate));

    UFO_PROCESSOR_CLASS (klass)->setup  = ufo_math_shrinkage_method_setup_real;
}