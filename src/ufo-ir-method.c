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

#include <ufo-ir-method.h>

static void ufo_method_interface_init (UfoMethodIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrMethod, ufo_ir_method, UFO_TYPE_PROCESSOR,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_METHOD,
                                                ufo_method_interface_init))

#define UFO_IR_METHOD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_IR_METHOD, UfoIrMethodPrivate))

struct _UfoIrMethodPrivate {
    UfoProjector *projector;
    gfloat  relaxation_factor;
    guint   max_iterations;
};

enum {
    PROP_0,
    PROP_PROJECTION_MODEL,
    PROP_RELAXATION_FACTOR,
    PROP_MAX_ITERATIONS,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoMethod *
ufo_ir_method_new ()
{
    return (UfoMethod *) g_object_new (UFO_TYPE_IR_METHOD,
                                       NULL);
}

static void
ufo_ir_method_set_property (GObject      *object,
                            guint        property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    UfoIrMethodPrivate *priv = UFO_IR_METHOD_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_PROJECTION_MODEL:
            g_clear_object(&priv->projector);
            priv->projector = g_object_ref (g_value_get_pointer (value));
            g_print ("\nufo_ir_method_set_property %p: PROP_PROJECTION_MODEL: %p", object, priv->projector);
            g_value_set_pointer (value, priv->projector);
            break;
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
        case PROP_PROJECTION_MODEL:
            g_print ("\nufo_ir_method_get_property %p: PROP_PROJECTION_MODEL: %p", object, priv->projector);
            g_value_set_pointer (value, priv->projector);
            break;
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
    g_clear_object(&priv->projector);
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

static void
ufo_ir_method_set_prior_knowledge_real (UfoIrMethod *method,
                                        GHashTable  *prior)
{
    g_warning ("Selected method does not work with any prior knowledge.");
}

static gboolean
ufo_ir_method_process_real (UfoMethod *method,
                            UfoBuffer *input,
                            UfoBuffer *output)
{
    warn_unimplemented (method, "process");
    return FALSE;
}

static void
ufo_method_interface_init (UfoMethodIface *iface)
{
    iface->process = ufo_ir_method_process_real;
}

static void
ufo_ir_method_class_init (UfoIrMethodClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = ufo_ir_method_finalize;
    gobject_class->dispose = ufo_ir_method_dispose;
    gobject_class->set_property = ufo_ir_method_set_property;
    gobject_class->get_property = ufo_ir_method_get_property;

    properties[PROP_PROJECTION_MODEL] =
        g_param_spec_pointer("projection-model",
                             "Pointer to the instance of UfoProjector.",
                             "Pointer to the instance of UfoProjector.",
                             G_PARAM_READWRITE);

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

    UFO_IR_METHOD_CLASS (klass)->set_prior_knowledge =
        ufo_ir_method_set_prior_knowledge_real;
    UFO_PROCESSOR_CLASS (klass)->setup = ufo_ir_method_setup_real;
}

void
ufo_ir_method_set_prior_knowledge (UfoIrMethod *method,
                                   GHashTable  *prior)
{
    g_return_if_fail (UFO_IS_IR_METHOD (method) && prior);
    UFO_IR_METHOD_GET_CLASS (method)->set_prior_knowledge(method, prior);
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