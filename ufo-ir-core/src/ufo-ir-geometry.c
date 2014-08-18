/*
* Copyright (C) 2011-2013 Karlsruhe Institute of Technology
*
* This file is part of Ufo.
*
* This library is free software: you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation, either
* version 3 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ufo-ir-geometry.h"
#include <math.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

G_DEFINE_TYPE (UfoIrGeometry, ufo_ir_geometry, G_TYPE_OBJECT)

#define UFO_IR_GEOMETRY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_GEOMETRY, UfoIrGeometryPrivate))

GQuark
ufo_ir_geometry_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-geometry-error-quark");
}

struct _UfoIrGeometryPrivate {
    UfoIrGeometryDims dimensions;

    guint   n_angles;
    gdouble angle_step;
    gdouble angle_offset;

    cl_context context;
    cl_mem scan_sin_lut;
    cl_mem scan_cos_lut;
    gfloat *scan_host_sin_lut;
    gfloat *scan_host_cos_lut;
};

enum {
    PROP_DIMENSIONS = N_IR_GEOMETRY_VIRTUAL_PROPERTIES,
    PROP_NUM_ANGLES,
    PROP_ANGLE_STEP,
    PROP_ANGLE_OFFSET,
    N_PROPERTIES
};

static GParamSpec *ufo_ir_geometry_props[N_PROPERTIES] = { NULL, };

static cl_mem
create_lut_buffer (UfoIrGeometryPrivate *priv,
                   gfloat               **host_mem,
                   double (*func)(double))
{
    cl_int errcode;
    guint n_entries = priv->n_angles;
    gsize size = n_entries * sizeof (gfloat);
    cl_mem mem = NULL;

    *host_mem = g_malloc0 (size);
    gdouble rad_angle = 0;
    for (guint i = 0; i < n_entries; i++) {
        rad_angle = priv->angle_offset + i * priv->angle_step * G_PI / 180.0f;
        (*host_mem)[i] = (gfloat) func (rad_angle);
    }

    mem = clCreateBuffer (priv->context,
                          CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
                          size, *host_mem,
                          &errcode);

    UFO_RESOURCES_CHECK_CLERR (errcode);
    return mem;
}

UfoIrGeometry *
ufo_ir_geometry_new (void)
{
    return UFO_IR_GEOMETRY(g_object_new (UFO_IR_TYPE_GEOMETRY, NULL));
}

static void
ufo_ir_geometry_setup_real (UfoIrGeometry *geometry,
                            UfoResources  *resources,
                            GError        **error)
{
    g_return_if_fail (UFO_IR_IS_GEOMETRY (geometry) &&
                      UFO_IS_RESOURCES (resources));

    UfoIrGeometryPrivate *priv = UFO_IR_GEOMETRY_GET_PRIVATE (geometry);
    priv->context = ufo_resources_get_context (resources);
    UFO_RESOURCES_CHECK_CLERR (clRetainContext (priv->context));
    priv->scan_sin_lut = create_lut_buffer (priv, &priv->scan_host_sin_lut, sin);
    priv->scan_cos_lut = create_lut_buffer (priv, &priv->scan_host_cos_lut, cos);
}

static void
ufo_ir_geometry_set_property (GObject      *object,
                              guint        property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
    UfoIrGeometryPrivate *priv = UFO_IR_GEOMETRY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_DIMENSIONS:
            priv->dimensions = *((UfoIrGeometryDims *)g_value_get_pointer(value));
            break;
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
ufo_ir_geometry_get_property (GObject    *object,
                              guint      property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
    UfoIrGeometryPrivate *priv = UFO_IR_GEOMETRY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_BEAM_GEOMETRY:
            g_value_set_string (value, "unknown");
            break;
        case PROP_DIMENSIONS:
            g_value_set_pointer (value, &priv->dimensions);
            break;
        case PROP_NUM_ANGLES:
            g_value_set_uint (value, priv->n_angles);
            break;
        case PROP_ANGLE_STEP:
            g_value_set_double (value, priv->angle_step);
            break;
        case PROP_ANGLE_OFFSET:
            g_value_set_double (value, priv->angle_offset);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_geometry_dispose (GObject *object)
{
    UfoIrGeometryPrivate *priv = UFO_IR_GEOMETRY_GET_PRIVATE (object);

    if (priv->scan_sin_lut) {
        UFO_RESOURCES_CHECK_CLERR (clReleaseMemObject (priv->scan_sin_lut));
        priv->scan_sin_lut = NULL;
    }

    if (priv->scan_cos_lut) {
        UFO_RESOURCES_CHECK_CLERR (clReleaseMemObject (priv->scan_cos_lut));
        priv->scan_cos_lut = NULL;
    }

    UFO_RESOURCES_CHECK_CLERR (clReleaseContext (priv->context));
    priv->context = NULL;

    G_OBJECT_CLASS (ufo_ir_geometry_parent_class)->dispose (object);
}

static void
ufo_ir_geometry_finalize (GObject *object)
{
    UfoIrGeometryPrivate *priv = UFO_IR_GEOMETRY_GET_PRIVATE (object);

    g_free (priv->scan_host_sin_lut);
    priv->scan_host_sin_lut = NULL;
    g_free (priv->scan_host_cos_lut);
    priv->scan_host_cos_lut = NULL;

    G_OBJECT_CLASS (ufo_ir_geometry_parent_class)->finalize (object);
}

static void
ufo_ir_geometry_configure_real (UfoIrGeometry  *geometry,
                                UfoRequisition *input_req,
                                GError         **error)
{
    UfoIrGeometryPrivate *priv = UFO_IR_GEOMETRY_GET_PRIVATE (geometry);

    priv->dimensions.n_dets = input_req->dims[0];
    priv->dimensions.n_angles = input_req->dims[1];

    if (priv->dimensions.n_angles > priv->n_angles) {
        g_message ("Actual number of directions is bigger than it was stated.");
    }
    else if (priv->dimensions.n_angles < priv->n_angles) {
        g_set_error (error, UFO_IR_GEOMETRY_ERROR, UFO_IR_GEOMETRY_ERROR_INPUT_DATA,
                     "Actual number of directions is less than it was stated.");
    }

}

static void
ufo_ir_geometry_get_volume_requisitions_real (UfoIrGeometry    *geometry,
                                              UfoRequisition *requisition)
{
    g_warning ("%s: `get_volume_requisitions' not implemented", G_OBJECT_TYPE_NAME (geometry));
}

static gpointer
ufo_ir_geometry_get_spec_real (UfoIrGeometry *geometry,
                               gsize *data_size)
{
    g_warning ("%s: `get_spec' not implemented", G_OBJECT_TYPE_NAME (geometry));
    return NULL;
}

static void
ufo_ir_geometry_class_init (UfoIrGeometryClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = ufo_ir_geometry_finalize;
    gobject_class->dispose = ufo_ir_geometry_dispose;
    gobject_class->set_property = ufo_ir_geometry_set_property;
    gobject_class->get_property = ufo_ir_geometry_get_property;

    const gfloat limit = (gfloat) (4.0 * G_PI);

    ufo_ir_geometry_props[PROP_BEAM_GEOMETRY] =
        g_param_spec_string ("beam-geometry",
                             "Geometry of the beam.",
                             "Geometry of the beam.",
                             "unknown",
                             G_PARAM_READABLE);

    ufo_ir_geometry_props[PROP_DIMENSIONS] =
        g_param_spec_pointer ("dimensions",
                              "Dimensions.",
                              "Dimensions.",
                              G_PARAM_READWRITE);

    ufo_ir_geometry_props[PROP_NUM_ANGLES] =
        g_param_spec_uint("num-angles",
                          "Number of angles",
                          "Number of angles",
                          0, G_MAXUINT, 0,
                          G_PARAM_READWRITE);

    ufo_ir_geometry_props[PROP_ANGLE_STEP] =
        g_param_spec_double ("angle-step",
                             "Increment of angle in radians.",
                             "Increment of angle in radians.",
                             -limit, +limit, 0.0,
                             G_PARAM_READWRITE);

    ufo_ir_geometry_props[PROP_ANGLE_OFFSET] =
        g_param_spec_double ("angle-offset",
                             "Angle offset in radians.",
                             "Angle offset in radians determining the first angle position.",
                             0.0, +limit, 0.0,
                             G_PARAM_READWRITE);

    for (guint i = GEOMETRY_PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, ufo_ir_geometry_props[i]);


    g_type_class_add_private (gobject_class, sizeof(UfoIrGeometryPrivate));

    UFO_IR_GEOMETRY_CLASS (klass)->configure = ufo_ir_geometry_configure_real;
    UFO_IR_GEOMETRY_CLASS (klass)->get_volume_requisitions =
      ufo_ir_geometry_get_volume_requisitions_real;
    UFO_IR_GEOMETRY_CLASS (klass)->get_spec = ufo_ir_geometry_get_spec_real;
    UFO_IR_GEOMETRY_CLASS (klass)->setup = ufo_ir_geometry_setup_real;
}

static void
ufo_ir_geometry_init(UfoIrGeometry *self)
{
    UfoIrGeometryPrivate *priv = NULL;
    self->priv = priv = UFO_IR_GEOMETRY_GET_PRIVATE(self);

    priv->n_angles = 0;
    priv->angle_step = G_PI;
    priv->angle_offset = 0.0f;

    priv->context = NULL;
    priv->scan_cos_lut = NULL;
    priv->scan_cos_lut = NULL;
    priv->scan_host_sin_lut = NULL;
    priv->scan_host_cos_lut = NULL;
}

gfloat *
ufo_ir_geometry_scan_angles_host (UfoIrGeometry *geometry,
                                  UfoAnglesType type)
{
    UfoIrGeometryPrivate *priv = UFO_IR_GEOMETRY_GET_PRIVATE (geometry);
    switch (type) {
      case SIN_VALUES:
          return priv->scan_host_sin_lut;
      case COS_VALUES:
          return priv->scan_host_cos_lut;
      default:
          return NULL;
    }
}

gpointer
ufo_ir_geometry_scan_angles_device (UfoIrGeometry *geometry,
                                    UfoAnglesType type)
{
    UfoIrGeometryPrivate *priv = UFO_IR_GEOMETRY_GET_PRIVATE (geometry);
    switch (type) {
      case SIN_VALUES:
          return priv->scan_sin_lut;
      case COS_VALUES:
          return priv->scan_cos_lut;
      default:
          return NULL;
    }
}

void
ufo_ir_geometry_configure (UfoIrGeometry  *geometry,
                           UfoRequisition *input_req,
                           GError         **error)
{
    g_return_if_fail (UFO_IR_IS_GEOMETRY (geometry) && input_req);
    UFO_IR_GEOMETRY_GET_CLASS (geometry)->configure(geometry,
                                                    input_req,
                                                    error);
}

void
ufo_ir_geometry_get_volume_requisitions (UfoIrGeometry    *geometry,
                                      UfoRequisition *requisition)
{
    g_return_if_fail (UFO_IR_IS_GEOMETRY (geometry));
    UFO_IR_GEOMETRY_GET_CLASS (geometry)->get_volume_requisitions(geometry,
                                                                  requisition);
}

void
ufo_ir_geometry_setup (UfoIrGeometry *geometry,
                       UfoResources  *resources,
                       GError        **error)
{
    g_return_if_fail (UFO_IR_IS_GEOMETRY (geometry) &&
                      UFO_IS_RESOURCES (resources));
    UFO_IR_GEOMETRY_GET_CLASS (geometry)->setup(geometry, resources, error);
}

gpointer
ufo_ir_geometry_get_spec (UfoIrGeometry *geometry,
                          gsize         *data_size)
{
    g_return_val_if_fail (UFO_IR_IS_GEOMETRY (geometry) && data_size, NULL);
    return UFO_IR_GEOMETRY_GET_CLASS (geometry)->get_spec(geometry, data_size);
}

static gchar *
transform_string (const gchar *pattern,
                  const gchar *s,
                  const gchar *separator)
{
    gchar **sv;
    gchar *transformed;
    gchar *result;
    sv = g_strsplit_set (s, "-_ ", -1);
    transformed = g_strjoinv (separator, sv);
    result = g_strdup_printf (pattern, transformed);
    g_strfreev (sv);
    g_free (transformed);
    return result;
}

gpointer
ufo_ir_geometry_from_json (JsonObject       *object,
                           UfoPluginManager *manager)
{
    GError *tmp_error = NULL;
    const gchar *plugin_name = json_object_get_string_member (object, "beam-geometry");
    const gchar *module_name = transform_string ("libufoir_%s_geometry.so", plugin_name, NULL);
    gchar *func_name = transform_string ("ufo_ir_%s_geometry_new", plugin_name, "_");
    gpointer plugin = ufo_plugin_manager_get_plugin (manager,
                                                     func_name,
                                                     module_name,
                                                     &tmp_error);
    if (tmp_error != NULL) {
      g_warning ("%s", tmp_error->message);
      return NULL;
    }

    GList *members = json_object_get_members (object);
    GList *member = NULL;
    gchar *name = NULL;
    for (member = g_list_first (members);
         member != NULL;
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