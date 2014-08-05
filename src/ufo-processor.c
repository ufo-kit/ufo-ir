#include <ufo-processor.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

G_DEFINE_TYPE (UfoProcessor, ufo_processor, G_TYPE_OBJECT)

#define UFO_PROCESSOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_PROCESSOR, UfoProcessorPrivate))
gboolean projector_type_error (UfoProcessor *self, GError **error);

GQuark
ufo_processor_error_quark (void)
{
    return g_quark_from_static_string ("ufo-processor-error-quark");
}

struct _UfoProcessorPrivate {
    UfoResources *resources;
    gpointer      cmd_queue;
};

enum {
    PROP_0,
    PROP_UFO_RESOURCES,
    PROP_CL_COMMAND_QUEUE,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoProcessor *
ufo_processor_new ()
{
    return (UfoProcessor *) g_object_new (UFO_TYPE_PROCESSOR,
                                       NULL);
}

static void
ufo_processor_init (UfoProcessor *self)
{
    UfoProcessorPrivate *priv = NULL;
    self->priv = priv = UFO_PROCESSOR_GET_PRIVATE (self);
    priv->resources = NULL;
    priv->cmd_queue = NULL;
}

static void
ufo_processor_set_property (GObject *object,
                            guint property_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
    UfoProcessorPrivate *priv = UFO_PROCESSOR_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_UFO_RESOURCES:
            if (priv->resources) {
                g_object_unref (priv->resources);
                priv->resources = NULL;
            }
            priv->resources = g_object_ref (g_value_get_pointer (value));
            break;
        case PROP_CL_COMMAND_QUEUE:
            if (priv->cmd_queue) {
                 UFO_RESOURCES_CHECK_CLERR (clReleaseCommandQueue (priv->cmd_queue));
            }
            priv->cmd_queue = g_value_get_pointer (value);
            UFO_RESOURCES_CHECK_CLERR (clRetainCommandQueue (priv->cmd_queue));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_processor_get_property (GObject *object,
                            guint property_id,
                            GValue *value,
                            GParamSpec *pspec)
{
    UfoProcessorPrivate *priv = UFO_PROCESSOR_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_UFO_RESOURCES:
            g_value_set_pointer (value, priv->resources);
            break;
        case PROP_CL_COMMAND_QUEUE:
            g_value_set_pointer (value, priv->cmd_queue);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_processor_dispose (GObject *object)
{
    UfoProcessorPrivate *priv = UFO_PROCESSOR_GET_PRIVATE (object);
    if (priv->resources) {
        g_object_unref(priv->resources);
    }

    G_OBJECT_CLASS (ufo_processor_parent_class)->dispose (object);
}

static void
ufo_processor_finalize (GObject *object)
{
     UfoProcessorPrivate *priv = UFO_PROCESSOR_GET_PRIVATE (object);
    if (priv->cmd_queue) {
        UFO_RESOURCES_CHECK_CLERR (clReleaseCommandQueue (priv->cmd_queue));
        priv->cmd_queue = NULL;
    }

    G_OBJECT_CLASS (ufo_processor_parent_class)->finalize (object);
}

static void
ufo_processor_setup_real (UfoProcessor *processor,
                          UfoResources *resources,
                          GError       **error)
{
    g_object_set (processor,
                  "ufo-resources", resources,
                  NULL);
}

static void
ufo_processor_class_init (UfoProcessorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = ufo_processor_finalize;
    gobject_class->dispose = ufo_processor_dispose;
    gobject_class->set_property = ufo_processor_set_property;
    gobject_class->get_property = ufo_processor_get_property;


    properties[PROP_UFO_RESOURCES] =
        g_param_spec_pointer("ufo-resources",
                             "Pointer to the instance of UfoResources.",
                             "Pointer to the instance of UfoResources.",
                             G_PARAM_READWRITE);

    properties[PROP_CL_COMMAND_QUEUE] =
        g_param_spec_pointer("command-queue",
                             "Pointer to the instance of cl_command_queue.",
                             "Pointer to the instance of cl_command_queue.",
                             G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);


    g_type_class_add_private (gobject_class, sizeof (UfoProcessorPrivate));
    klass->setup = ufo_processor_setup_real;
}

void
ufo_processor_setup (UfoProcessor *processor,
                     UfoResources *resources,
                     GError       **error)
{
    g_print ("\n ufo_processor_setup \n");
    g_return_if_fail(UFO_IS_PROCESSOR (processor) &&
                     UFO_IS_RESOURCES (resources));

    UfoProcessorClass *klass = UFO_PROCESSOR_GET_CLASS (processor);
    g_object_set (processor, "ufo-resources", resources, NULL);
    g_print ("\n ufo_processor_setup: %p %p \n", processor, resources);
    klass->setup (processor, resources, error);
}
