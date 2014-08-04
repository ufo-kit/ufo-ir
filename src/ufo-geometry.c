#include <ufo-geometry.h>
#include <math.h>

G_DEFINE_TYPE (UfoGeometry, ufo_geometry, G_TYPE_OBJECT)

#define UFO_GEOMETRY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_GEOMETRY, UfoGeometryPrivate))

GQuark
ufo_geometry_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-geometry-error-quark");
}

struct _UfoGeometryPrivate {
    cl_context context;

    guint  n_angles;
    gdouble angle_step;
    gdouble angle_offset;

    cl_mem scan_sin_lut;
    cl_mem scan_cos_lut;
    gfloat *scan_host_sin_lut;
    gfloat *scan_host_cos_lut;
};

enum {
    PROP_0,
    PROP_NUM_ANGLES,
    PROP_ANGLE_STEP,
    PROP_ANGLE_OFFSET,
    PROP_BEAM_GEOMETRY,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

static cl_mem
create_lut_buffer (UfoGeometryPrivate *priv,
                   gfloat             **host_mem,
                   double (*func)(double))
{
    cl_int errcode;
    guint n_entries = priv->n_angles;
    gsize size = n_entries * sizeof (gfloat);
    cl_mem mem = NULL;

    *host_mem = g_malloc0 (size);

    for (guint i = 0; i < n_entries; i++)
        (*host_mem)[i] = (gfloat) func (priv->angle_offset + i * priv->angle_step);

    mem = clCreateBuffer (priv->context,
                          CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
                          size, *host_mem,
                          &errcode);

    UFO_RESOURCES_CHECK_CLERR (errcode);
    return mem;
}

UfoGeometry *
ufo_geometry_new ()
{
    return UFO_GEOMETRY(g_object_new (UFO_TYPE_GEOMETRY, NULL));
}

static void
ufo_geometry_setup_real (UfoGeometry  *geometry,
                         UfoResources *resources,
                         GError       **error)
{
    g_return_if_fail (UFO_IS_GEOMETRY (geometry) &&
                      UFO_IS_RESOURCES (resources));

    UfoGeometryPrivate *priv = UFO_GEOMETRY_GET_PRIVATE (geometry);
    priv->context = ufo_resources_get_context (resources);
    UFO_RESOURCES_CHECK_CLERR (clRetainContext (priv->context));

    priv->scan_sin_lut = create_lut_buffer (priv, &priv->scan_host_sin_lut, sin);
    priv->scan_cos_lut = create_lut_buffer (priv, &priv->scan_host_cos_lut, cos);
}

static void
ufo_geometry_set_property (GObject      *object,
                           guint        property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    UfoGeometryPrivate *priv = UFO_GEOMETRY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_NUM_ANGLES:
            priv->n_angles = g_value_get_uint(value);
            break;
        case PROP_ANGLE_STEP:
            priv->angle_step = g_value_get_double(value);
            break;
        case PROP_ANGLE_OFFSET:
            priv->angle_offset = g_value_get_double(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_geometry_get_property (GObject    *object,
                           guint      property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    UfoGeometryPrivate *priv = UFO_GEOMETRY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_NUM_ANGLES:
            g_value_set_uint (value, priv->n_angles);
            break;
        case PROP_ANGLE_STEP:
            g_value_set_double (value, priv->angle_step);
            break;
        case PROP_ANGLE_OFFSET:
            g_value_set_double (value, priv->angle_offset);
            break;
        case PROP_BEAM_GEOMETRY: {
            gchar* geom = UFO_GEOMETRY_GET_CLASS(object)->beam_geometry(UFO_GEOMETRY(object));
            geom = geom ? geom : "unknown";
            g_value_set_string (value, geom);
          }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_geometry_finalize (GObject *object)
{
    UfoGeometryPrivate *priv = UFO_GEOMETRY_GET_PRIVATE (object);

    if (priv->scan_sin_lut) {
        UFO_RESOURCES_CHECK_CLERR (clReleaseMemObject (priv->scan_sin_lut));
        priv->scan_sin_lut = NULL;
    }

    if (priv->scan_cos_lut) {
        UFO_RESOURCES_CHECK_CLERR (clReleaseMemObject (priv->scan_cos_lut));
        priv->scan_cos_lut = NULL;
    }

    g_free (priv->scan_host_sin_lut);
    g_free (priv->scan_host_cos_lut);

    UFO_RESOURCES_CHECK_CLERR (clReleaseContext (priv->context));
    priv->context = NULL;

    G_OBJECT_CLASS (ufo_geometry_parent_class)->finalize (object);
}

static void
ufo_geometry_get_volume_requisitions_real (UfoGeometry     *geometry,
                                           UfoRequisition  *requisition)
{
  warn_unimplemented (geometry, "get_volume_requisitions");
}

static gchar*
ufo_geometry_beam_geometry_real (UfoGeometry *geometry)
{
  warn_unimplemented (geometry, "beam_geometry");
}

gsize
ufo_geometry_get_meta_real (UfoGeometry *geometry,
                            gpointer    *meta)
{
  warn_unimplemented (geometry, "get_meta");
  *meta = NULL;
  return 0;
}

static void
ufo_geometry_class_init (UfoGeometryClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = ufo_geometry_finalize;
    gobject_class->set_property = ufo_geometry_set_property;
    gobject_class->get_property = ufo_geometry_get_property;

    const gfloat limit = (gfloat) (4.0 * G_PI);

    properties[PROP_NUM_ANGLES] =
        g_param_spec_uint("num-angles",
                          "Number of angles",
                          "Number of angles",
                          0, G_MAXUINT, 0,
                          G_PARAM_READWRITE);

    properties[PROP_ANGLE_STEP] =
        g_param_spec_double ("angle-step",
                             "Increment of angle in radians.",
                             "Increment of angle in radians.",
                             -limit, +limit, 0.0,
                             G_PARAM_READWRITE);

    properties[PROP_ANGLE_OFFSET] =
        g_param_spec_double ("angle-offset",
                             "Angle offset in radians.",
                             "Angle offset in radians determining the first angle position.",
                             0.0, +limit, 0.0,
                             G_PARAM_READWRITE);

    properties[PROP_BEAM_GEOMETRY] =
        g_param_spec_string ("beam-geometry",
                             "Geometry of the beam.",
                             "Geometry of the beam.",
                             "unknown",
                             G_PARAM_READABLE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);


    g_type_class_add_private (gobject_class, sizeof(UfoGeometryPrivate));

    UFO_GEOMETRY_CLASS (klass)->get_volume_requisitions = ufo_geometry_get_volume_requisitions_real;
    UFO_GEOMETRY_CLASS (klass)->beam_geometry = ufo_geometry_beam_geometry_real;
    UFO_GEOMETRY_CLASS (klass)->get_meta = ufo_geometry_get_meta_real;
    UFO_GEOMETRY_CLASS (klass)->setup = ufo_geometry_setup_real;
}

static void
ufo_geometry_init(UfoGeometry *self)
{
    UfoGeometryPrivate *priv = NULL;
    self->priv = priv = UFO_GEOMETRY_GET_PRIVATE(self);

    priv->context = NULL;
    priv->n_angles = 0;
    priv->angle_step = G_PI;
    priv->angle_offset = 0.0f;

    priv->scan_cos_lut = NULL;
    priv->scan_cos_lut = NULL;
    priv->scan_host_sin_lut = NULL;
    priv->scan_host_cos_lut = NULL;
}

gfloat *
ufo_geometry_scan_angles_host (UfoGeometry *geometry,
                               UfoAnglesType type)
{
    UfoGeometryPrivate *priv = UFO_GEOMETRY_GET_PRIVATE (geometry);

    if (type == SIN_VALUES)
        return priv->scan_host_sin_lut;
    else if (type == COS_VALUES)
        return priv->scan_host_cos_lut;
    else
        return NULL;
}

gpointer
ufo_geometry_scan_angles_device (UfoGeometry *geometry,
                                 UfoAnglesType type)
{
    UfoGeometryPrivate *priv = UFO_GEOMETRY_GET_PRIVATE (geometry);

    if (type == SIN_VALUES)
        return priv->scan_sin_lut;
    else if (type == COS_VALUES)
        return priv->scan_cos_lut;
    else
        return NULL;
}

void
ufo_geometry_get_volume_requisitions (UfoGeometry    *geometry,
                                      UfoBuffer      *measurements,
                                      UfoRequisition *requisition,
                                      GError         **error)
{
    g_return_if_fail (UFO_IS_GEOMETRY (geometry));
    UFO_GEOMETRY_GET_CLASS (geometry)->get_volume_requisitions(geometry,
                                                               measurements,
                                                               requisition,
                                                               error);
}

void
ufo_geometry_setup (UfoGeometry  *geometry,
                    UfoResources *resources,
                    GError       **error)
{
    g_return_if_fail (UFO_IS_GEOMETRY (geometry) &&
                      UFO_IS_RESOURCES (resources));
    UFO_GEOMETRY_GET_CLASS (geometry)->setup(geometry, resources, error);
}

gsize
ufo_geometry_get_meta (UfoGeometry *geometry,
                       gpointer    *meta)
{
    g_return_val_if_fail (UFO_IS_GEOMETRY (geometry) && meta, 0);
    return UFO_GEOMETRY_GET_CLASS (geometry)->get_meta(geometry, meta);
}

gpointer
ufo_geometry_from_json (JsonObject       *object,
                        UfoPluginManager *manager,
                        GError           **error)
{
    gchar *plugin_name = json_object_get_string_member (object, "beam-geometry");
    /*gpointer plugin = ufo_plugin_manager_get_plugin (manager,
                                            METHOD_FUNC_NAME, // depends on method categeory
                                            plugin_name,
                                            error);*/
    gpointer plugin = ufo_parallel_geometry_new ();

    GList *members = json_object_get_members (object);
    GList *member = NULL;
    gchar *name = NULL;
    for (member = g_list_first (members);
         member != NULL && !error;
         member = g_list_next (member))
    {
        name = member->data;
        if (g_strcmp0 (name, "beam-geometry") == 0) {;
          continue;
        }

        JsonNode *node = json_object_get_member(object, name);
        if (JSON_NODE_HOLDS_VALUE (node)) {
            GValue val = {0,};
            json_node_get_value (node, &val);
            g_object_set_property (G_OBJECT(plugin), name, &val);
            g_value_unset (&val);
        }
        else {
            g_warning ("`%s' is not a primitive value!", name);
        }
    }

    return plugin;
}