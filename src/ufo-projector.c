#include <ufo-projector.h>
#include <ufo-geometry.h>

G_DEFINE_TYPE (UfoProjector, ufo_projector, G_TYPE_OBJECT)

#define UFO_PROJECTOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_PROJECTOR, UfoProjectorPrivate))
gboolean projector_type_error (UfoProjector *self, GError **error);

GQuark
ufo_projector_error_quark (void)
{
    return g_quark_from_static_string ("ufo-projector-error-quark");
}

struct _UfoProjectorPrivate {
    UfoGeometry *geometry;
};

enum {
    PROP_0,
    PROP_GEOMETRY,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoProjector *
ufo_projector_new ()
{
    return UFO_PROJECTOR (g_object_new (UFO_TYPE_PROJECTOR, NULL));
}

static void
_ufo_projector_setup (UfoProjector *projector,
                      UfoResources *resources,
                      GError       **error)
{
    UfoProjectorPrivate *priv = UFO_PROJECTOR_GET_PRIVATE (projector);
    if (!priv->geometry) {
        g_set_error (error, UFO_PROJECTOR_ERROR, UFO_PROJECTOR_ERROR_SETUP,
                     "Property ::geometry is not specified");
    }
}

static void
ufo_projector_set_property (GObject      *object,
                            guint        property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    UfoProjectorPrivate *priv = UFO_PROJECTOR_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_GEOMETRY:
            priv->geometry = g_object_ref (g_value_get_pointer(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_projector_get_property (GObject    *object,
                            guint      property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
    UfoProjectorPrivate *priv = UFO_PROJECTOR_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_GEOMETRY:
            g_value_set_pointer (value, priv->geometry);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_projector_dispose (GObject *object)
{
    UfoProjectorPrivate *priv = UFO_PROJECTOR_GET_PRIVATE (object);
    g_object_unref(priv->geometry);

    G_OBJECT_CLASS (ufo_projector_parent_class)->dispose (object);
}

static void
_ufo_projector_FP_ROI (UfoProjector         *projector,
                       UfoBuffer            *volume,
                       UfoRegion            *volume_roi,
                       UfoBuffer            *measurements,
                       UfoProjectionsSubset subset,
                       gfloat               scale,
                       cl_event             *finish_event)
{
    g_warning ("Method FP_ROI is not implemented.");
}

static void
_ufo_projector_BP_ROI (UfoProjector         *projector,
                       UfoBuffer            *volume,
                       UfoRegion            *volume_roi,
                       UfoBuffer            *measurements,
                       UfoProjectionsSubset subset,
                       cl_event             *finish_event)
{
    g_warning ("Method BP_ROI is not implemented.");
}


static void
ufo_projector_class_init (UfoProjectorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = ufo_projector_dispose;
    gobject_class->set_property = ufo_projector_set_property;
    gobject_class->get_property = ufo_projector_get_property;

    properties[PROP_GEOMETRY] =
        g_param_spec_pointer("geometry",
                             "Pointer to the instance of UfoGeometry structure",
                             "Pointer to the instance of UfoGeometry structure",
                             G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);


    g_type_class_add_private (gobject_class, sizeof(UfoProjectorPrivate));

    klass->FP_ROI = _ufo_projector_FP_ROI;
    klass->BP_ROI = _ufo_projector_BP_ROI;
    klass->setup  = _ufo_projector_setup;
}

void
ufo_projector_FP_ROI (UfoProjector         *projector,
                      UfoBuffer            *volume,
                      UfoRegion            *volume_roi,
                      UfoBuffer            *measurements,
                      UfoProjectionsSubset subset,
                      gfloat               scale,
                      cl_event             *finish_event)
{
    g_return_if_fail (UFO_IS_PROJECTOR (projector) &&
                      UFO_IS_BUFFER (volume) &&
                      volume_roi &&
                      UFO_IS_BUFFER (measurements));

    UfoProjectorClass *klass = UFO_PROJECTOR_GET_CLASS (projector);
    klass->FP_ROI(projector,
                  volume, volume_roi,
                  measurements, subset,
                  scale, finish_event);
}

void
ufo_projector_BP_ROI (UfoProjector         *projector,
                      UfoBuffer            *volume,
                      UfoRegion            *volume_roi,
                      UfoBuffer            *measurements,
                      UfoProjectionsSubset subset,
                      cl_event             *finish_event)
{
    g_return_if_fail (UFO_IS_PROJECTOR (projector) &&
                      UFO_IS_BUFFER (volume) &&
                      volume_roi &&
                      UFO_IS_BUFFER (measurements));

    UfoProjectorClass *klass = UFO_PROJECTOR_GET_CLASS (projector);
    klass->BP_ROI(projector,
                  volume, volume_roi,
                  measurements, subset,
                  finish_event);
}

void
ufo_projector_FP (UfoProjector         *projector,
                  UfoBuffer            *volume,
                  UfoBuffer            *measurements,
                  UfoProjectionsSubset subset,
                  gfloat               scale,
                  cl_event             *finish_event)
{
    UfoRequisition volume_req;
    ufo_buffer_get_requisition (volume, &volume_req);

    UfoRegion region;
    for (int i = 0; i < UFO_BUFFER_MAX_NDIMS; ++i) {
      region.origin[i] = 0;
      region.size[i] = volume_req.dims[i];
    }

    ufo_projector_FP_ROI (projector,
                          volume,
                          &region,
                          measurements,
                          subset,
                          scale,
                          finish_event);
}

void
ufo_projector_BP (UfoProjector         *projector,
                  UfoBuffer            *volume,
                  UfoBuffer            *measurements,
                  UfoProjectionsSubset subset,
                  cl_event             *finish_event)
{
    UfoRequisition volume_req;
    ufo_buffer_get_requisition (volume, &volume_req);

    UfoRegion region;
    for (int i = 0; i < UFO_BUFFER_MAX_NDIMS; ++i) {
      region.origin[i] = 0;
      region.size[i] = volume_req.dims[i];
    }

    ufo_projector_BP_ROI (projector,
                          volume,
                          &region,
                          measurements,
                          subset,
                          finish_event);
}

void
ufo_projector_setup (UfoProjector  *projector,
                     UfoResources  *resources,
                     GError        **error)
{
    g_return_if_fail (UFO_IS_PROJECTOR (projector) &&
                      UFO_IS_RESOURCES (resources));

    UfoProjectorClass *klass = UFO_PROJECTOR_GET_CLASS (projector);
    klass->setup(projector, resources, error);
}

static void
ufo_projector_init (UfoProjector *self)
{
    UfoProjectorPrivate *priv = NULL;
    self->priv = priv = UFO_PROJECTOR_GET_PRIVATE (self);
    priv->geometry = NULL;
}