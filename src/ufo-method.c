#include <ufo-method.h>

G_DEFINE_TYPE (UfoMethod, ufo_method, G_TYPE_OBJECT)

#define UFO_METHOD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_METHOD, UfoMethodPrivate))
gboolean projector_type_error (UfoMethod *self, GError **error);

GQuark
ufo_method_error_quark (void)
{
    return g_quark_from_static_string ("ufo-method-error-quark");
}

struct _UfoMethodPrivate {
    UfoResources *resources;
};

enum {
    PROP_0,
    PROP_UFO_RESOURCES,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoMethod *
ufo_method_new ()
{
    return (UfoMethod *) g_object_new (UFO_TYPE_METHOD,
                                       NULL);
}

static void
ufo_method_init (UfoMethod *self)
{
    UfoMethodPrivate *priv = NULL;
    self->priv = priv = UFO_METHOD_GET_PRIVATE (self);
    priv->resources = NULL;
}

static void
ufo_method_set_property (GObject *object,
                         guint property_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
    UfoMethodPrivate *priv = UFO_METHOD_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_UFO_RESOURCES:
            priv->resources = g_object_ref (g_value_get_pointer (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_method_get_property (GObject *object,
                         guint property_id,
                         GValue *value,
                         GParamSpec *pspec)
{
    UfoMethodPrivate *priv = UFO_METHOD_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_UFO_RESOURCES:
            g_value_set_pointer (value, priv->resources);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_method_dispose (GObject *object)
{
    UfoMethodPrivate *priv = UFO_METHOD_GET_PRIVATE (object);
    g_object_unref(priv->resources);
    G_OBJECT_CLASS (ufo_method_parent_class)->dispose (object);
}

static void
ufo_method_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_method_parent_class)->finalize (object);
}

static void
_ufo_method_setup (UfoMethod  *method,
                   UfoResources *resources,
                   GError       **error)
{
    /*g_object_set (method,
                  "ufo-resources", resources,
                  NULL);
                  */
}


static void
_ufo_method_process (UfoMethod  *method,
                     UfoBuffer *input,
                     UfoBuffer *output)
{
    g_error ("'process' is not implemented.");
}

static void
ufo_method_class_init (UfoMethodClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = ufo_method_finalize;
    gobject_class->dispose = ufo_method_dispose;
    gobject_class->set_property = ufo_method_set_property;
    gobject_class->get_property = ufo_method_get_property;


    properties[PROP_UFO_RESOURCES] =
        g_param_spec_pointer("ufo-resources",
                             "Pointer to the instance of UfoResources.",
                             "Pointer to the instance of UfoResources.",
                             G_PARAM_READWRITE);


    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);


    g_type_class_add_private (gobject_class, sizeof (UfoMethodPrivate));
    klass->setup = _ufo_method_setup;
    klass->process = _ufo_method_process;
}

void
ufo_method_setup (UfoMethod  *method,
                  UfoResources *resources,
                  GError       **error)
{
    //g_return_if_fail(UFO_IS_METHOD (method) &&
    //                 UFO_IS_RESOURCES (resources));

    UfoMethodClass *klass = UFO_METHOD_GET_CLASS (method);
    klass->setup (method, resources, error);
}

void
ufo_method_process (UfoMethod  *method,
                    UfoBuffer *input,
                    UfoBuffer *output)
{
    g_return_if_fail(UFO_IS_BUFFER (input) &&
                     UFO_IS_BUFFER (output));

    UfoMethodClass *klass = UFO_METHOD_GET_CLASS (method);
    klass->process (method, input, output);
}
