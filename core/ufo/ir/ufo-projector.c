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

#include <ufo/ir/ufo-projector.h>
#include <ufo/ir/ufo-geometry.h>

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
    UfoProfiler *profiler;
    UfoRegion   full_volume_region;
};

enum {
    PROP_0,
    PROP_GEOMETRY,
    PROP_UFO_PROFILER,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoProjector *
ufo_projector_new (void)
{
    return UFO_PROJECTOR (g_object_new (UFO_TYPE_PROJECTOR, NULL));
}

static void
ufo_projector_setup_real (UfoProjector *projector,
                          UfoResources *resources,
                          GError       **error)
{
    UfoProjectorPrivate *priv = UFO_PROJECTOR_GET_PRIVATE (projector);
    if (!priv->geometry) {
        g_set_error (error, UFO_PROJECTOR_ERROR, UFO_PROJECTOR_ERROR_SETUP,
                     "%s : property \"geometry\" is not specified.",
                     G_OBJECT_TYPE_NAME (projector));
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
            g_clear_object(&priv->geometry);
            priv->geometry = g_object_ref (g_value_get_pointer(value));
            break;
        case PROP_UFO_PROFILER:
            g_clear_object(&priv->profiler);
            priv->profiler = g_object_ref (g_value_get_pointer (value));
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
        case PROP_UFO_PROFILER:
            g_value_set_pointer (value, priv->profiler);
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
    g_clear_object(&priv->geometry);

    G_OBJECT_CLASS (ufo_projector_parent_class)->dispose (object);
}

static void
ufo_projector_FP_ROI_real (UfoProjector         *projector,
                           UfoBuffer            *volume,
                           UfoRegion            *volume_roi,
                           UfoBuffer            *measurements,
                           UfoProjectionsSubset *subset,
                           gfloat               scale,
                           gpointer             finish_event)
{
    g_warning ("%s: `FP_ROI' not implemented", G_OBJECT_TYPE_NAME (projector));
}

static void
ufo_projector_BP_ROI_real (UfoProjector         *projector,
                           UfoBuffer            *volume,
                           UfoRegion            *volume_roi,
                           UfoBuffer            *measurements,
                           UfoProjectionsSubset *subset,
                           gfloat               relax_param,
                           gpointer             finish_event)
{
    g_warning ("%s: `BP_ROI' not implemented", G_OBJECT_TYPE_NAME (projector));
}

static void
ufo_projector_configure_real (UfoProjector *projector)
{
    UfoProjectorPrivate *priv = UFO_PROJECTOR_GET_PRIVATE (projector);

    UfoRequisition req;
    ufo_geometry_get_volume_requisitions (priv->geometry, &req);
    for (int i = 0; i < UFO_BUFFER_MAX_NDIMS; ++i) {
        priv->full_volume_region.origin[i] = 0;
        priv->full_volume_region.size[i] = req.dims[i];
    }
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

    properties[PROP_UFO_PROFILER] =
        g_param_spec_pointer("ufo-profiler",
                             "Pointer to the instance of UfoProfiler.",
                             "Pointer to the instance of UfoProfiler.",
                             G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);


    g_type_class_add_private (gobject_class, sizeof(UfoProjectorPrivate));

    klass->FP_ROI = ufo_projector_FP_ROI_real;
    klass->BP_ROI = ufo_projector_BP_ROI_real;
    klass->setup  = ufo_projector_setup_real;
    klass->configure = ufo_projector_configure_real;
}

void
ufo_projector_FP_ROI (UfoProjector         *projector,
                      UfoBuffer            *volume,
                      UfoRegion            *volume_roi,
                      UfoBuffer            *measurements,
                      UfoProjectionsSubset *subset,
                      gfloat               scale,
                      gpointer             finish_event)
{
    g_return_if_fail (UFO_IS_PROJECTOR (projector) &&
                      UFO_IS_BUFFER (volume) &&
                      volume_roi &&
                      UFO_IS_BUFFER (measurements) &&
                      subset);

    if (scale == 0.0f) {
      g_warning("%s : FP argument scale = 0. Operation skipped.",
                G_OBJECT_TYPE_NAME (projector));
    }

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
                      UfoProjectionsSubset *subset,
                      gfloat               relax_param,
                      gpointer             finish_event)
{
    g_return_if_fail (UFO_IS_PROJECTOR (projector) &&
                      UFO_IS_BUFFER (volume) &&
                      volume_roi &&
                      UFO_IS_BUFFER (measurements) &&
                      subset);

    if (relax_param == 0.0f) {
      g_warning("%s : BP argument relax_param = 0. Operation skipped.",
                G_OBJECT_TYPE_NAME (projector));
    }

    UfoProjectorClass *klass = UFO_PROJECTOR_GET_CLASS (projector);
    klass->BP_ROI(projector,
                  volume, volume_roi,
                  measurements, subset,
                  relax_param,
                  finish_event);
}

void
ufo_projector_FP (UfoProjector         *projector,
                  UfoBuffer            *volume,
                  UfoBuffer            *measurements,
                  UfoProjectionsSubset *subset,
                  gfloat               scale,
                  gpointer             finish_event)
{
    UfoProjectorPrivate *priv = UFO_PROJECTOR_GET_PRIVATE (projector);
    ufo_projector_FP_ROI (projector,
                          volume,
                          &priv->full_volume_region,
                          measurements,
                          subset,
                          scale,
                          finish_event);
}

void
ufo_projector_BP (UfoProjector         *projector,
                  UfoBuffer            *volume,
                  UfoBuffer            *measurements,
                  UfoProjectionsSubset *subset,
                  gfloat               relax_param,
                  gpointer             finish_event)
{
    UfoProjectorPrivate *priv = UFO_PROJECTOR_GET_PRIVATE (projector);
    ufo_projector_BP_ROI (projector,
                          volume,
                          &priv->full_volume_region,
                          measurements,
                          subset,
                          relax_param,
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

void
ufo_projector_configure (UfoProjector *projector)
{
    g_return_if_fail (UFO_IS_PROJECTOR (projector));
    UfoProjectorClass *klass = UFO_PROJECTOR_GET_CLASS (projector);
    klass->configure(projector);
}

static void
ufo_projector_init (UfoProjector *self)
{
    UfoProjectorPrivate *priv = NULL;
    self->priv = priv = UFO_PROJECTOR_GET_PRIVATE (self);
    priv->geometry = NULL;
    priv->profiler = NULL;
}

gpointer
ufo_projector_from_json (JsonObject       *object,
                         UfoPluginManager *manager)
{
  #if 0
    gboolean gpu = json_object_get_boolean_member (object, "on-gpu");
    gchar *model = json_object_get_string_member (object, "model");
    gpointer plugin = NULL;

    if (gpu) {
        /*
        plugin = ufo_plugin_manager_get_plugin (manager,
                                                METHOD_FUNC_NAME,
                                                // depends on method categeory
                                                plugin_name,
                                                error);*/
        plugin = ufo_cl_projector_new ();
        g_object_set (plugin, "model", model, NULL);
        g_print ("\nProjector: %p", plugin);
    }
    else {
      // plugin-name = model+geometry.type = joseph-parallel
    }

    return plugin;
  #endif
    return NULL;
}