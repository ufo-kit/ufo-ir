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

#include "ufo-ir-method.h"
#include "ufo-ir-projector.h"
#include "ufo-ir-geometry.h" 

/**
* SECTION:ufo-ir-method
* @Short_description: Base class for an iterative reconstruction method.
* @Title: UfoIrMethod
*/

static void ufo_method_interface_init (UfoMethodIface *iface);
static void ufo_copyable_interface_init (UfoCopyableIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrMethod, ufo_ir_method, UFO_TYPE_PROCESSOR,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_METHOD,
                                                ufo_method_interface_init)
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_COPYABLE,
                                                ufo_copyable_interface_init))

#define UFO_IR_METHOD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_METHOD, UfoIrMethodPrivate))

struct _UfoIrMethodPrivate {
    UfoIrProjector *projector;
    gfloat  relaxation_factor;
    guint   max_iterations;
};

enum {
    PROP_0 = 0,
    PROP_RELAXATION_FACTOR,
    PROP_MAX_ITERATIONS,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

/**
* ufo_ir_method_new:
*
* Create a #UfoIrMethod object.
*
* Return value: (transfer full): A new #UfoIrMethod object.
*/
UfoMethod *
ufo_ir_method_new (void)
{
    return (UfoMethod *) g_object_new (UFO_IR_TYPE_METHOD, NULL);
}


void
ufo_ir_method_set_projection_model (UfoIrMethod *method,
                                    gpointer    projector)
{
    if (projector == NULL || !UFO_IR_IS_PROJECTOR (projector))
        return;

    UfoIrMethodPrivate *priv = UFO_IR_METHOD_GET_PRIVATE (method);
    if (priv->projector)
        g_object_unref (priv->projector);
    
    priv->projector = g_object_ref (projector);
}


/**
 * ufo_ir_method_get_projection_model:
 * @method: A #UfoIrMethod
 * 
 * Returns: A pointer on #UfoIrProjector
 */
gpointer
ufo_ir_method_get_projection_model (UfoIrMethod *method)
{
    UfoIrMethodPrivate *priv = UFO_IR_METHOD_GET_PRIVATE (method);
    return priv->projector;
}

static void
ufo_ir_method_set_property (GObject      *object,
                            guint        property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    UfoIrMethodPrivate *priv = UFO_IR_METHOD_GET_PRIVATE (object);

    switch (property_id) {
       case PROP_RELAXATION_FACTOR:
            priv->relaxation_factor = g_value_get_float (value);
            break;
        case PROP_MAX_ITERATIONS:
            priv->max_iterations = g_value_get_uint (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_method_get_property (GObject    *object,
                            guint      property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
    UfoIrMethodPrivate *priv = UFO_IR_METHOD_GET_PRIVATE (object);

    switch (property_id) {
       case PROP_RELAXATION_FACTOR:
            g_value_set_float (value, priv->relaxation_factor);
            break;
        case PROP_MAX_ITERATIONS:
            g_value_set_uint (value, priv->max_iterations);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_method_dispose (GObject *object)
{
    UfoIrMethodPrivate *priv = UFO_IR_METHOD_GET_PRIVATE (object);

    if (priv->projector) {
        g_object_unref (priv->projector);
        priv->projector = NULL;
    }

    G_OBJECT_CLASS (ufo_ir_method_parent_class)->dispose (object);
}

static void
ufo_ir_method_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_ir_method_parent_class)->finalize (object);
}

static void
ufo_ir_method_setup_real (UfoProcessor *processor,
                          UfoResources *resources,
                          GError       **error)
{
    UFO_PROCESSOR_CLASS (ufo_ir_method_parent_class)->setup (processor, resources, error);
}

static gboolean
ufo_ir_method_process_real (UfoMethod *method,
                            UfoBuffer *input,
                            UfoBuffer *output,
                            gpointer  pevent)
{
    g_warning ("%s: `process' not implemented", G_OBJECT_TYPE_NAME (method));
    return FALSE;
}

static void
ufo_method_interface_init (UfoMethodIface *iface)
{
    iface->process = ufo_ir_method_process_real;
}

static UfoCopyable *
ufo_ir_method_copy_real (gpointer origin,
                         gpointer _copy)
{
    g_print ("\nufo_ir_method_copy_real (%p %p", origin, _copy);
    UfoCopyable *copy;
    if (_copy)
        copy = UFO_COPYABLE(_copy);
    else
        copy = UFO_COPYABLE (ufo_ir_method_new());


    UfoIrMethodPrivate * priv = UFO_IR_METHOD_GET_PRIVATE (origin);
    gpointer prj_copy = ufo_copyable_copy (priv->projector, NULL);
    if (prj_copy) {
        ufo_ir_method_set_projection_model (UFO_IR_METHOD(copy), prj_copy);
        g_object_unref (G_OBJECT(prj_copy));
    }

    g_object_set (G_OBJECT(copy),
                  "relaxation_factor", priv->relaxation_factor,
                  "max-iterations", priv->max_iterations,
                  NULL);

    return copy;
}

static void
ufo_copyable_interface_init (UfoCopyableIface *iface)
{
    iface->copy = ufo_ir_method_copy_real;
}

static void
ufo_ir_method_class_init (UfoIrMethodClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = ufo_ir_method_finalize;
    gobject_class->dispose = ufo_ir_method_dispose;
    gobject_class->set_property = ufo_ir_method_set_property;
    gobject_class->get_property = ufo_ir_method_get_property;

    properties[PROP_RELAXATION_FACTOR] =
        g_param_spec_float("relaxation-factor",
                           "The relaxation parameter.",
                           "The relaxation parameter.",
                           0.0f, G_MAXFLOAT, 1.0f,
                           G_PARAM_READWRITE);

    properties[PROP_MAX_ITERATIONS] =
        g_param_spec_uint("max-iterations",
                          "The number of maximum iterations.",
                          "The number of maximum iterations.",
                          0, G_MAXUINT, 1,
                          G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);


    g_type_class_add_private (gobject_class, sizeof (UfoIrMethodPrivate));

    UFO_PROCESSOR_CLASS (klass)->setup = ufo_ir_method_setup_real;
}

static void
ufo_ir_method_init (UfoIrMethod *self)
{
    UfoIrMethodPrivate *priv = NULL;
    self->priv = priv = UFO_IR_METHOD_GET_PRIVATE (self);
    priv->projector = NULL;
    priv->relaxation_factor = 1.0f;
    priv->max_iterations = 1;
}
