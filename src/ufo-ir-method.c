#include <ufo-ir-method.h>

G_DEFINE_TYPE (UfoIrMethod, ufo_ir_method, UFO_TYPE_METHOD)

#define UFO_IR_METHOD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_IR_METHOD, UfoIrMethodPrivate))

GQuark
ufo_ir_method_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-method-error-quark");
}

struct _UfoIrMethodPrivate {
    //UfoProjector *projector;
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

UfoIrMethod *
ufo_ir_method_new ()
{
    return (UfoIrMethod *) g_object_new (UFO_TYPE_IR_METHOD,
                                         NULL);
}

static void
ufo_ir_method_init (UfoIrMethod *self)
{
    UfoIrMethodPrivate *priv = NULL;
    self->priv = priv = UFO_IR_METHOD_GET_PRIVATE (self);
}

static void
ufo_ir_method_set_property (GObject *object,
                                 guint property_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
    UfoIrMethodPrivate *priv = UFO_IR_METHOD_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_PROJECTION_MODEL:
            //priv->projector = g_object_ref (g_value_get_pointer (value));
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
ufo_ir_method_get_property (GObject *object,
                            guint property_id,
                            GValue *value,
                            GParamSpec *pspec)
{
    UfoIrMethodPrivate *priv = UFO_IR_METHOD_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_PROJECTION_MODEL:
            //g_value_set_pointer (value, priv->projector);
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
    //g_object_unref(priv->projector);
    G_OBJECT_CLASS (ufo_ir_method_parent_class)->dispose (object);
}

static void
ufo_ir_method_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_ir_method_parent_class)->finalize (object);
}

static void
_ufo_ir_method_setup (UfoIrMethod  *method,
                      UfoResources *resources,
                      GError       **error)
{
    UFO_METHOD_CLASS (ufo_ir_method_parent_class)->setup (method, resources, error);
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

    UFO_METHOD_CLASS (klass)->setup = _ufo_ir_method_setup;
}