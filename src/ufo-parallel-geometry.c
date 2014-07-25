#include <ufo-parallel-geometry.h>
#include <math.h>

G_DEFINE_TYPE (UfoParallelGeometry, ufo_parallel_geometry, UFO_TYPE_GEOMETRY)

#define UFO_PARALLEL_GEOMETRY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_GEOMETRY, UfoParallelGeometryPrivate))

GQuark
ufo_parallel_geometry_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-geometry-error-quark");
}

struct _UfoParallelGeometryPrivate {
    gfloat detector_scale;
    gfloat detector_offset;
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
            priv->detector_scale = g_value_get_float(value);
            break;
        case PROP_DETECTOR_OFFSET:
            priv->detector_offset = g_value_get_float(value);
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
            g_value_set_float (value, priv->detector_scale);
            break;
        case PROP_DETECTOR_OFFSET:
            g_value_set_float (value, priv->detector_offset);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
_ufo_parallel_geometry_get_volume_requisitions (UfoGeometry     *geometry,
                                                UfoRequisition  *requisition)
{
    g_print ("\n _ufo_parallel_geometry_get_volume_requisitions \n");
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
                            0.1f, 4.0f, 0.0f,
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

    UFO_GEOMETRY_CLASS(klass)->get_volume_requisitions = _ufo_parallel_geometry_get_volume_requisitions;
}

static void
ufo_parallel_geometry_init(UfoParallelGeometry *self)
{
    UfoParallelGeometryPrivate *priv = NULL;
    self->priv = priv = UFO_PARALLEL_GEOMETRY_GET_PRIVATE(self);

    priv->detector_scale = 1.0f;
    priv->detector_offset = 0.0f;
}