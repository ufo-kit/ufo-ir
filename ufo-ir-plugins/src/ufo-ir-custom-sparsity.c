#include "ufo-ir-custom-sparsity.h"

static void ufo_ir_sparsity_interface_init (UfoIrSparsityIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrCustomSparsity, ufo_ir_custom_sparsity, UFO_TYPE_PROCESSOR,
                         G_IMPLEMENT_INTERFACE (UFO_IR_TYPE_SPARSITY,
                                                ufo_ir_sparsity_interface_init))

#define UFO_IR_CUSTOM_SPARSITY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_CUSTOM_SPARSITY, UfoIrCustomSparsityPrivate))

GQuark
ufo_ir_custom_sparsity_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-custom-sparsity-error-quark");
}

struct _UfoIrCustomSparsityPrivate {
    UfoMethod    *method;
    UfoTransform *transform;
};

enum {
    PROP_0,
    PROP_METHOD,
    PROP_TRANSFORM,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoIrSparsity *
ufo_ir_custom_sparsity_new (void)
{
    return UFO_IR_SPARSITY (g_object_new (UFO_IR_TYPE_CUSTOM_SPARSITY, NULL));
}

static void
ufo_ir_custom_sparsity_set_property (GObject      *object,
                                     guint        property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
    UfoIrCustomSparsityPrivate *priv = UFO_IR_CUSTOM_SPARSITY_GET_PRIVATE (object);

    GObject *value_object;

    switch (property_id) {
        case PROP_METHOD:
            {
                value_object = g_value_get_object (value);

                if (priv->method)
                    g_object_unref (priv->method);

                if (value_object != NULL) {
                    priv->method = g_object_ref (UFO_METHOD (value_object));
                }
            }
            break;
        case PROP_TRANSFORM:
            {
                value_object = g_value_get_object (value);

                if (priv->transform)
                    g_object_unref (priv->transform);

                if (value_object != NULL) {
                    priv->transform = g_object_ref (UFO_TRANSFORM (value_object));
                }
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_custom_sparsity_get_property (GObject    *object,
                                     guint      property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
    UfoIrCustomSparsityPrivate *priv = UFO_IR_CUSTOM_SPARSITY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_METHOD:
            g_value_set_object (value, priv->method);
            break;
        case PROP_TRANSFORM:
            g_value_set_object (value, priv->transform);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_custom_sparsity_dispose (GObject *object)
{
    UfoIrCustomSparsityPrivate *priv = UFO_IR_CUSTOM_SPARSITY_GET_PRIVATE (object);

    if (priv->transform) {
        g_object_unref (priv->transform);
        priv->transform = NULL;
    }

    if (priv->method) {
        g_object_unref (priv->method);
        priv->method = NULL;
    }

    G_OBJECT_CLASS (ufo_ir_custom_sparsity_parent_class)->dispose (object);
}

static void
ufo_ir_custom_sparsity_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_ir_custom_sparsity_parent_class)->finalize (object);
}

static void
ufo_ir_custom_sparsity_setup_real (UfoProcessor *processor,
                                   UfoResources *resources,
                                   GError       **error)
{
    UFO_PROCESSOR_CLASS (ufo_ir_custom_sparsity_parent_class)->setup (processor,
                                                                      resources,
                                                                      error);
}

static gboolean
ufo_ir_custom_sparsity_minimize_real (UfoIrSparsity *sparsity,
                                      UfoBuffer     *input,
                                      UfoBuffer     *output,
                                      gpointer      pevent)
{
    UfoIrCustomSparsityPrivate *priv = UFO_IR_CUSTOM_SPARSITY_GET_PRIVATE (sparsity);

    UfoBuffer *tmp = ufo_buffer_dup (output);
    UfoBuffer *tmp2 = ufo_buffer_dup (output);
    ufo_transform_direct (priv->transform, input, tmp, NULL);
    ufo_method_process (priv->method, tmp, tmp2, NULL);
    ufo_transform_inverse (priv->transform, tmp2, output, NULL);

    return FALSE;
}

static void
ufo_ir_sparsity_interface_init (UfoIrSparsityIface *iface)
{
    iface->minimize = ufo_ir_custom_sparsity_minimize_real;
}

static void
ufo_ir_custom_sparsity_class_init (UfoIrCustomSparsityClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = ufo_ir_custom_sparsity_finalize;
    gobject_class->dispose = ufo_ir_custom_sparsity_dispose;
    gobject_class->set_property = ufo_ir_custom_sparsity_set_property;
    gobject_class->get_property = ufo_ir_custom_sparsity_get_property;

    properties[PROP_METHOD] =
        g_param_spec_object("method",
                            "Pointer to the instance of UfoMethod.",
                            "Pointer to the instance of UfoMethod.",
                            UFO_TYPE_METHOD,
                            G_PARAM_READWRITE);

    properties[PROP_TRANSFORM] =
        g_param_spec_object("transform",
                            "Pointer to the instance of UfoTransform.",
                            "Pointer to the instance of UfoTransform.",
                            UFO_TYPE_TRANSFORM,
                            G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);


    g_type_class_add_private (gobject_class, sizeof (UfoIrCustomSparsityPrivate));
    UFO_PROCESSOR_CLASS (klass)->setup = ufo_ir_custom_sparsity_setup_real;
}

static void
ufo_ir_custom_sparsity_init (UfoIrCustomSparsity *self)
{
    UfoIrCustomSparsityPrivate *priv = NULL;
    self->priv = priv = UFO_IR_CUSTOM_SPARSITY_GET_PRIVATE (self);
    priv->method = NULL;
    priv->transform = NULL;
}