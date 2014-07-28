#include <ufo-cl-projector.h>
#include <ufo-geometry.h>

G_DEFINE_TYPE (UfoClProjector, ufo_cl_projector, UFO_TYPE_PROJECTOR)

#define UFO_CL_PROJECTOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_CL_PROJECTOR, UfoClProjectorPrivate))

GQuark
ufo_cl_projector_error_quark (void)
{
    return g_quark_from_static_string ("ufo-cl-projector-error-quark");
}

struct _UfoClProjectorPrivate {
  gpointer cmd_queue;

  gchar    *model_name;
  gpointer fp_kernel[2];
  gpointer bp_kernel;
};

enum {
    PROP_0,
    PROP_MODEL_NAME,
    PROP_CL_COMMAND_QUEUE,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoProjector *
ufo_cl_projector_new ()
{
    return UFO_PROJECTOR (g_object_new (UFO_TYPE_CL_PROJECTOR, NULL));
}

static void
ufo_cl_projector_setup_real (UfoProjector *projector,
                             UfoResources *resources,
                             GError       **error)
{
    //UfoClProjectorPrivate *priv = UFO_CL_PROJECTOR_GET_PRIVATE (projector);
    // get kernels
    gchar *model = NULL;
    UfoGeometry *geometry = NULL;
    g_object_get (projector, "geometry", &geometry,
                             "model", &model, NULL);

    gchar *beam_geometry = NULL;
    g_object_get (geometry, "beam-geometry", &beam_geometry, NULL);

    gchar *cl_filename = g_strconcat("projector", beam_geometry, model, NULL);

    g_print ("\nCL Filename: %s", cl_filename);

}

static void
ufo_cl_projector_set_property (GObject      *object,
                               guint        property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
    UfoClProjectorPrivate *priv = UFO_CL_PROJECTOR_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_CL_COMMAND_QUEUE:
            if (priv->cmd_queue) {
                UFO_RESOURCES_CHECK_CLERR (clReleaseCommandQueue (priv->cmd_queue));
            }
            priv->cmd_queue = g_value_get_pointer(value);
            UFO_RESOURCES_CHECK_CLERR (clRetainCommandQueue (priv->cmd_queue));
            break;
        case PROP_MODEL_NAME:
            g_free (priv->model_name);
            priv->model_name = g_value_dup_string (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_cl_projector_get_property (GObject    *object,
                               guint      property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
    UfoClProjectorPrivate *priv = UFO_CL_PROJECTOR_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_CL_COMMAND_QUEUE:
            g_value_set_pointer (value, priv->cmd_queue);
            break;
        case PROP_MODEL_NAME:
            g_value_set_string (value, priv->model_name);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_cl_projector_dispose (GObject *object)
{
  //  UfoClProjectorPrivate *priv = UFO_CL_PROJECTOR_GET_PRIVATE (object);
  //  g_object_unref(priv->geometry);

    G_OBJECT_CLASS (ufo_cl_projector_parent_class)->dispose (object);
}

static void
ufo_cl_projector_finalize (GObject *gobject)
{
    UfoClProjectorPrivate *priv = UFO_CL_PROJECTOR_GET_PRIVATE (gobject);

    if (priv->fp_kernel) {
        UFO_RESOURCES_CHECK_CLERR (clReleaseKernel (priv->fp_kernel[Horizontal]));
        priv->fp_kernel[Horizontal] = NULL;

        UFO_RESOURCES_CHECK_CLERR (clReleaseKernel (priv->fp_kernel[Vertical]));
        priv->fp_kernel[Vertical] = NULL;
    }

    if (priv->bp_kernel) {
        UFO_RESOURCES_CHECK_CLERR (clReleaseKernel (priv->bp_kernel));
        priv->bp_kernel = NULL;
    }

    if (priv->cmd_queue) {
        UFO_RESOURCES_CHECK_CLERR (clReleaseCommandQueue (priv->cmd_queue));
        priv->cmd_queue = NULL;
    }

    G_OBJECT_CLASS(ufo_cl_projector_parent_class)->finalize(gobject);
}

static void
ufo_cl_projector_FP_ROI_real (UfoProjector         *projector,
                              UfoBuffer            *volume,
                              UfoRegion            *volume_roi,
                              UfoBuffer            *measurements,
                              UfoProjectionsSubset subset,
                              gfloat               scale,
                              cl_event             *finish_event)
{

}

static void
ufo_cl_projector_BP_ROI_real (UfoProjector         *projector,
                              UfoBuffer            *volume,
                              UfoRegion            *volume_roi,
                              UfoBuffer            *measurements,
                              UfoProjectionsSubset subset,
                              cl_event             *finish_event)
{

}

static void
ufo_cl_projector_class_init (UfoClProjectorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = ufo_cl_projector_dispose;
    gobject_class->finalize = ufo_cl_projector_finalize;
    gobject_class->set_property = ufo_cl_projector_set_property;
    gobject_class->get_property = ufo_cl_projector_get_property;


    properties[PROP_MODEL_NAME] =
        g_param_spec_string ("model-name",
                             "The name of the projection model.",
                             "The name of the projection model.",
                             "joseph",
                              G_PARAM_READWRITE);

    properties[PROP_CL_COMMAND_QUEUE] =
        g_param_spec_pointer("command-queue",
                             "Pointer to the instance of cl_command_queue.",
                             "Pointer to the instance of cl_command_queue.",
                             G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof(UfoClProjectorPrivate));

    UfoProjectorClass *projector_class = UFO_PROJECTOR_CLASS(klass);
    projector_class->FP_ROI = ufo_cl_projector_FP_ROI_real;
    projector_class->BP_ROI = ufo_cl_projector_BP_ROI_real;
    projector_class->setup  = ufo_cl_projector_setup_real;
}

static void
ufo_cl_projector_init (UfoClProjector *self)
{
    UfoClProjectorPrivate *priv = NULL;
    self->priv = priv = UFO_CL_PROJECTOR_GET_PRIVATE (self);
    priv->model_name = g_strdup ("joseph");
    priv->cmd_queue = NULL;
}