#include <ufo-parallel-geometry.h>
#include <ufo-cl-parallel-geometry.h>
#include <math.h>

G_DEFINE_TYPE (UfoParallelGeometry, ufo_parallel_geometry, UFO_TYPE_GEOMETRY)

#define UFO_PARALLEL_GEOMETRY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_GEOMETRY, UfoParallelGeometryPrivate))

GQuark
ufo_parallel_geometry_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-geometry-error-quark");
}

struct _UfoParallelGeometryPrivate {
    UfoParallelGeometryMeta meta;
};

enum {
    PROP_0,
    PROP_DETECTOR_SCALE,
    PROP_DETECTOR_OFFSET,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoGeometry *
ufo_parallel_geometry_new ()
{
    return UFO_GEOMETRY(g_object_new (UFO_TYPE_PARALLEL_GEOMETRY, NULL));
}

static void
ufo_parallel_geometry_set_property (GObject      *object,
                                    guint        property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
    UfoParallelGeometryPrivate *priv = UFO_PARALLEL_GEOMETRY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_DETECTOR_SCALE:
            priv->meta.detector_scale = g_value_get_float(value);
            break;
        case PROP_DETECTOR_OFFSET:
            priv->meta.detector_offset = g_value_get_float(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_parallel_geometry_get_property (GObject    *object,
                                    guint      property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
    UfoParallelGeometryPrivate *priv = UFO_PARALLEL_GEOMETRY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_DETECTOR_SCALE:
            g_value_set_float (value, priv->meta.detector_scale);
            break;
        case PROP_DETECTOR_OFFSET:
            g_value_set_float (value, priv->meta.detector_offset);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_parallel_geometry_get_volume_requisitions_real (UfoGeometry    *geometry,
                                                    UfoBuffer      *measurements,
                                                    UfoRequisition *requisition,
                                                    GError         **error)
{
    g_print ("\nufo_parallel_geometry_get_volume_requisitions_real \n");
    UfoParallelGeometryPrivate *priv = UFO_PARALLEL_GEOMETRY_GET_PRIVATE (geometry);

    UfoRequisition req;
    ufo_buffer_get_requisition (measurements, &req);
    requisition->n_dims = req.n_dims;
    requisition->dims[0] = req.dims[0] * (gsize) ceil (priv->meta.detector_scale);
    requisition->dims[1] = req.dims[0] * (gsize) ceil (priv->meta.detector_scale);
    requisition->dims[2] = req.dims[0] * (gsize) ceil (priv->meta.detector_scale);

    guint n_angles;
    g_object_get (geometry, "num-angles", &n_angles, NULL);
    if (req.dims[1] > n_angles) {
        g_message ("Actual number of directions is bigger than it was stated.");
    }
    else if (req.dims[1] < n_angles) {
        g_set_error (error, UFO_GEOMETRY_ERROR, UFO_GEOMETRY_ERROR_INPUT_DATA,
                     "Actual number of directions is less than it was stated.");
    }
}

static const gchar*
ufo_parallel_geometry_beam_geometry_real (UfoGeometry *geometry)
{
    return "parallel";
}

static gsize
ufo_parallel_geometry_get_meta_real (UfoGeometry *geometry,
                                     gpointer    *meta,
                                     GError      **error)
{
    UfoParallelGeometryPrivate *priv = UFO_PARALLEL_GEOMETRY_GET_PRIVATE (geometry);
    *meta = g_malloc (sizeof (UfoParallelGeometryMeta));
    //**meta = priv->meta;
    // memcopy instead
    return sizeof (UfoParallelGeometryMeta);
}

static void
ufo_parallel_geometry_class_init (UfoParallelGeometryClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->set_property = ufo_parallel_geometry_set_property;
    gobject_class->get_property = ufo_parallel_geometry_get_property;

    const gfloat limit = (gfloat) (4.0 * G_PI);

    properties[PROP_DETECTOR_SCALE] =
        g_param_spec_float ("detector-scale",
                            "Aspect ratio of pixel size and detector size.",
                            "Aspect ratio of pixel size and detector size.",
                            0.1f, 4.0f, 1.0f,
                            G_PARAM_READWRITE);

    properties[PROP_DETECTOR_OFFSET] =
        g_param_spec_float ("detector-offset",
                            "Shift of the detectors respect to the center of the volume.",
                            "Shift of the detectors respect to the center of the volume.",
                            -limit, +limit, 0.0f,
                            G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof(UfoParallelGeometryPrivate));

    UFO_GEOMETRY_CLASS(klass)->get_volume_requisitions = ufo_parallel_geometry_get_volume_requisitions_real;
    UFO_GEOMETRY_CLASS(klass)->beam_geometry = ufo_parallel_geometry_beam_geometry_real;
    UFO_GEOMETRY_CLASS(klass)->get_meta = ufo_parallel_geometry_get_meta_real;
}

static void
ufo_parallel_geometry_init(UfoParallelGeometry *self)
{
    UfoParallelGeometryPrivate *priv = NULL;
    self->priv = priv = UFO_PARALLEL_GEOMETRY_GET_PRIVATE(self);

    priv->meta.detector_scale = 1.0f;
    priv->meta.detector_offset = 0.0f;
}