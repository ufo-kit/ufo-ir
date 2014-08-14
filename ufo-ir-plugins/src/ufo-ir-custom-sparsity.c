#include "ufo-ir-custom-sparsity.h"

static void ufo_sparsity_interface_init (UfoSparsityIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoCustomSparsity, ufo_custom_sparsity, UFO_TYPE_PROCESSOR,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_SPARSITY,
                                                ufo_sparsity_interface_init))

#define UFO_CUSTOM_SPARSITY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_CUSTOM_SPARSITY, UfoCustomSparsityPrivate))

GQuark
ufo_custom_sparsity_error_quark (void)
{
    return g_quark_from_static_string ("ufo-custom-sparsity-error-quark");
}

struct _UfoCustomSparsityPrivate {
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

UfoSparsity *
ufo_custom_sparsity_new (void)
{
    return (UfoSparsity *) g_object_new (UFO_TYPE_CUSTOM_SPARSITY,
                                         NULL);
}

static void
ufo_custom_sparsity_set_property (GObject *object,
                                  guint property_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
    UfoCustomSparsityPrivate *priv = UFO_CUSTOM_SPARSITY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_METHOD:
            priv->method = g_object_ref (g_value_get_pointer (value));
            break;
        case PROP_TRANSFORM:
            priv->transform = g_object_ref (g_value_get_pointer (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_custom_sparsity_get_property (GObject *object,
                                  guint property_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
    UfoCustomSparsityPrivate *priv = UFO_CUSTOM_SPARSITY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_METHOD:
            g_value_set_pointer (value, priv->method);
            break;
        case PROP_TRANSFORM:
            g_value_set_pointer (value, priv->transform);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_custom_sparsity_dispose (GObject *object)
{
    UfoCustomSparsityPrivate *priv = UFO_CUSTOM_SPARSITY_GET_PRIVATE (object);
    g_object_unref (priv->transform);
    g_object_unref (priv->method);
    G_OBJECT_CLASS (ufo_custom_sparsity_parent_class)->dispose (object);
}

static void
ufo_custom_sparsity_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_custom_sparsity_parent_class)->finalize (object);
}

static void
ufo_custom_sparsity_setup_real (UfoProcessor *processor,
                                UfoResources *resources,
                                GError       **error)
{
    UFO_PROCESSOR_CLASS (ufo_custom_sparsity_parent_class)->setup (processor, resources, error);
}

static gboolean
ufo_custom_sparsity_minimize_real (UfoSparsity *sparsity,
                                   UfoBuffer *input,
                                   UfoBuffer *output)
{
    g_print ("\nufo_custom_sparsity_minimize_real\n");

    UfoCustomSparsityPrivate *priv = UFO_CUSTOM_SPARSITY_GET_PRIVATE (sparsity);

    UfoBuffer *tmp = ufo_buffer_dup (output);
    UfoBuffer *tmp2 = ufo_buffer_dup (output);
    ufo_transform_direct (priv->transform, input, tmp);
    ufo_method_process (priv->method, tmp, tmp2);
    ufo_transform_inverse (priv->transform, tmp2, output);

    return FALSE;
}

static void
ufo_sparsity_interface_init (UfoSparsityIface *iface)
{
    iface->minimize = ufo_custom_sparsity_minimize_real;
}

static void
ufo_custom_sparsity_class_init (UfoCustomSparsityClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = ufo_custom_sparsity_finalize;
    gobject_class->dispose = ufo_custom_sparsity_dispose;
    gobject_class->set_property = ufo_custom_sparsity_set_property;
    gobject_class->get_property = ufo_custom_sparsity_get_property;

    properties[PROP_METHOD] =
        g_param_spec_pointer("method",
                             "Pointer to the instance of UfoMethod.",
                             "Pointer to the instance of UfoMethod.",
                             G_PARAM_READWRITE);

    properties[PROP_TRANSFORM] =
        g_param_spec_pointer("transform",
                             "Pointer to the instance of UfoTransform.",
                             "Pointer to the instance of UfoTransform.",
                             G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);


    g_type_class_add_private (gobject_class, sizeof (UfoCustomSparsityPrivate));

    UFO_PROCESSOR_CLASS (klass)->setup = ufo_custom_sparsity_setup_real;
}

static void
ufo_custom_sparsity_init (UfoCustomSparsity *self)
{
    UfoCustomSparsityPrivate *priv = NULL;
    self->priv = priv = UFO_CUSTOM_SPARSITY_GET_PRIVATE (self);
    priv->method = NULL;
    priv->transform = NULL;
}