#include <ufo-gradient-sparsity.h>

static void ufo_sparsity_interface_init (UfoSparsityIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoGradientSparsity, ufo_gradient_sparsity, UFO_TYPE_PROCESSOR,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_SPARSITY,
                                                ufo_sparsity_interface_init))

#define UFO_GRADIENT_SPARSITY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_GRADIENT_SPARSITY, UfoGradientSparsityPrivate))

struct _UfoGradientSparsityPrivate {
    guint n_iters;
};

enum {
    PROP_0,
    PROP_NUM_ITERS,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoSparsity *
ufo_gradient_sparsity_new ()
{
    return (UfoSparsity *) g_object_new (UFO_TYPE_GRADIENT_SPARSITY,
                                         NULL);
}

static void
ufo_gradient_sparsity_set_property (GObject *object,
                                    guint property_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
    UfoGradientSparsityPrivate *priv = UFO_GRADIENT_SPARSITY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_NUM_ITERS:
            priv->n_iters = g_value_get_uint (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_gradient_sparsity_get_property (GObject *object,
                                    guint property_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
    UfoGradientSparsityPrivate *priv = UFO_GRADIENT_SPARSITY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_NUM_ITERS:
            g_value_set_uint (value, priv->n_iters);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_gradient_sparsity_dispose (GObject *object)
{
    //UfoGradientSparsityPrivate *priv = UFO_GRADIENT_SPARSITY_GET_PRIVATE (object);
    G_OBJECT_CLASS (ufo_gradient_sparsity_parent_class)->dispose (object);
}

static void
ufo_gradient_sparsity_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_gradient_sparsity_parent_class)->finalize (object);
}

static void
ufo_gradient_sparsity_setup_real (UfoProcessor *processor,
                                  UfoResources *resources,
                                  GError       **error)
{
    UFO_PROCESSOR_CLASS (ufo_gradient_sparsity_parent_class)->setup (processor, resources, error);
}

static gboolean
ufo_gradient_sparsity_minimize_real (UfoSparsity *sparsity,
                                     UfoBuffer *input,
                                     UfoBuffer *output)
{
    g_print ("\nufo_gradient_sparsity_minimize_real\n");
    return FALSE;
}

static void
ufo_sparsity_interface_init (UfoSparsityIface *iface)
{
    iface->minimize = ufo_gradient_sparsity_minimize_real;
}

static void
ufo_gradient_sparsity_class_init (UfoGradientSparsityClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = ufo_gradient_sparsity_finalize;
    gobject_class->dispose = ufo_gradient_sparsity_dispose;
    gobject_class->set_property = ufo_gradient_sparsity_set_property;
    gobject_class->get_property = ufo_gradient_sparsity_get_property;

    properties[PROP_NUM_ITERS] =
        g_param_spec_uint ("num-iters",
                           "Number of iterations",
                           "Number of iterations",
                           0, 1000, 20,
                           G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);


    g_type_class_add_private (gobject_class, sizeof (UfoGradientSparsityPrivate));

    UFO_PROCESSOR_CLASS (klass)->setup = ufo_gradient_sparsity_setup_real;
}

static void
ufo_gradient_sparsity_init (UfoGradientSparsity *self)
{
    UfoGradientSparsityPrivate *priv = NULL;
    self->priv = priv = UFO_GRADIENT_SPARSITY_GET_PRIVATE (self);
    priv->n_iters = 20;
}