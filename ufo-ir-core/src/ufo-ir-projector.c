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

#include "ufo-ir-projector.h"
#include "ufo-ir-geometry.h"

/**
* SECTION:ufo-ir-projector
* @Short_description: A base class for the projection models.
* @Title: UfoIrProjector
*
* A #UfoIrProjector stores common properties that all projection models use and
* provide an interface.
*/


static void ufo_copyable_interface_init (UfoCopyableIface *iface);
G_DEFINE_TYPE_WITH_CODE (UfoIrProjector, ufo_ir_projector, UFO_TYPE_PROCESSOR,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_COPYABLE,
                                                ufo_copyable_interface_init))

#define UFO_IR_PROJECTOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_PROJECTOR, UfoIrProjectorPrivate))
gboolean projector_type_error (UfoIrProjector *self, GError **error);

GQuark
ufo_ir_projector_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-projector-error-quark");
}

struct _UfoIrProjectorPrivate {
    UfoIrGeometry *geometry;
    UfoRegion     full_volume_region;
};

enum {
    PROP_0,
    PROP_GEOMETRY,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoIrProjector *
ufo_ir_projector_new (void)
{
    return UFO_IR_PROJECTOR (g_object_new (UFO_IR_TYPE_PROJECTOR, NULL));
}

static void
ufo_ir_projector_setup_real (UfoProcessor *projector,
                             UfoResources *resources,
                             GError       **error)
{
    UFO_PROCESSOR_CLASS (ufo_ir_projector_parent_class)->setup (projector, resources, error);
    if (error && *error)
      return;

    UfoIrProjectorPrivate *priv = UFO_IR_PROJECTOR_GET_PRIVATE (projector);
    if (!priv->geometry) {
        g_set_error (error, UFO_IR_PROJECTOR_ERROR, UFO_IR_PROJECTOR_ERROR_SETUP,
                     "%s : property \"geometry\" is not specified.",
                     G_OBJECT_TYPE_NAME (projector));
    }
}

static void
ufo_ir_projector_set_property (GObject      *object,
                               guint        property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
    UfoIrProjectorPrivate *priv = UFO_IR_PROJECTOR_GET_PRIVATE (object);

    GObject *value_object;

    switch (property_id) {
        case PROP_GEOMETRY:
            {
                value_object = g_value_get_object (value);

                if (priv->geometry)
                    g_object_unref (priv->geometry);

                if (value_object != NULL) {
                    priv->geometry = g_object_ref (UFO_IR_GEOMETRY (value_object));
                }
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_projector_get_property (GObject    *object,
                               guint      property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
    UfoIrProjectorPrivate *priv = UFO_IR_PROJECTOR_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_GEOMETRY:
            g_value_set_object (value, priv->geometry);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_projector_dispose (GObject *object)
{
    UfoIrProjectorPrivate *priv = UFO_IR_PROJECTOR_GET_PRIVATE (object);

    if (priv->geometry) {
        g_object_unref (priv->geometry);
        priv->geometry = NULL;
    }

    G_OBJECT_CLASS (ufo_ir_projector_parent_class)->dispose (object);
}

static void
ufo_ir_projector_FP_ROI_real (UfoIrProjector         *projector,
                              UfoBuffer              *volume,
                              UfoRegion              *volume_roi,
                              UfoBuffer              *measurements,
                              UfoIrProjectionsSubset *subset,
                              gfloat                 scale,
                              gpointer               finish_event)
{
    g_warning ("%s: `FP_ROI' not implemented", G_OBJECT_TYPE_NAME (projector));
}

static void
ufo_ir_projector_BP_ROI_real (UfoIrProjector         *projector,
                              UfoBuffer              *volume,
                              UfoRegion              *volume_roi,
                              UfoBuffer              *measurements,
                              UfoIrProjectionsSubset *subset,
                              gfloat                 relax_param,
                              gpointer               finish_event)
{
    g_warning ("%s: `BP_ROI' not implemented", G_OBJECT_TYPE_NAME (projector));
}

static void
ufo_ir_projector_configure_real (UfoProcessor *projector)
{
    UfoIrProjectorPrivate *priv = UFO_IR_PROJECTOR_GET_PRIVATE (projector);

    UfoRequisition req;
    ufo_ir_geometry_get_volume_requisitions (priv->geometry, &req);
    for (int i = 0; i < UFO_BUFFER_MAX_NDIMS; ++i) {
        priv->full_volume_region.origin[i] = 0;
        priv->full_volume_region.size[i] = req.dims[i];
    }
}

static UfoCopyable *
ufo_ir_projector_copy_real (gpointer origin,
                            gpointer _copy)
{
    UfoCopyable *copy;
    if (_copy)
        copy = UFO_COPYABLE(_copy);
    else
        copy = UFO_COPYABLE (ufo_ir_projector_new());

    UfoIrProjectorPrivate * priv = UFO_IR_PROJECTOR_GET_PRIVATE (origin);
    gpointer geom_copy = ufo_copyable_copy (priv->geometry, NULL);
    if (geom_copy) {
        g_object_set (G_OBJECT(copy), "geometry", geom_copy, NULL);
        g_object_unref (geom_copy);
    }
    return copy;
}

static void
ufo_copyable_interface_init (UfoCopyableIface *iface)
{
    iface->copy = ufo_ir_projector_copy_real;
}

static void
ufo_ir_projector_class_init (UfoIrProjectorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = ufo_ir_projector_dispose;
    gobject_class->set_property = ufo_ir_projector_set_property;
    gobject_class->get_property = ufo_ir_projector_get_property;

    properties[PROP_GEOMETRY] =
        g_param_spec_object ("geometry",
                             "Pointer to the instance of UfoIrGeometry structure",
                             "Pointer to the instance of UfoIrGeometry structure",
                             UFO_IR_TYPE_GEOMETRY,
                             G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);


    g_type_class_add_private (gobject_class, sizeof(UfoIrProjectorPrivate));

    klass->FP_ROI    = ufo_ir_projector_FP_ROI_real;
    klass->BP_ROI    = ufo_ir_projector_BP_ROI_real;
    UFO_PROCESSOR_CLASS (klass)->setup     = ufo_ir_projector_setup_real;
    UFO_PROCESSOR_CLASS (klass)->configure = ufo_ir_projector_configure_real;
}

/**
* ufo_ir_projector_FP_ROI:
* @projector: A #UfoIrProjector.
* @volume: A #UfoBuffer that describes the volume.
* @volume_roi: A #UfoRegion. It describes the ROI of the volume.
* @measurements: A #UfoBuffer that describes the measurement. It will be updated.
* @subset: A #UfoIrProjectionsSubset describes a projections subset, which
* should be projected.
* @scale: A #gfloat is a scaling coefficient.
* @finish_event: A #gpointer to cl_event that defines the operation end.
*
* Performs forward-projection of the ROI region in the volume.
*/
void
ufo_ir_projector_FP_ROI (UfoIrProjector         *projector,
                         UfoBuffer              *volume,
                         UfoRegion              *volume_roi,
                         UfoBuffer              *measurements,
                         UfoIrProjectionsSubset *subset,
                         gfloat                 scale,
                         gpointer               finish_event)
{
    g_return_if_fail (UFO_IR_IS_PROJECTOR (projector) &&
                      UFO_IS_BUFFER (volume) &&
                      volume_roi &&
                      UFO_IS_BUFFER (measurements) &&
                      subset);

    if (scale == 0.0f) {
      g_warning("%s : FP argument scale = 0. Operation skipped.",
                G_OBJECT_TYPE_NAME (projector));
    }

    UfoIrProjectorClass *klass = UFO_IR_PROJECTOR_GET_CLASS (projector);
    klass->FP_ROI(projector,
                  volume, volume_roi,
                  measurements, subset,
                  scale, finish_event);
}

/**
* ufo_ir_projector_BP_ROI:
* @projector: A #UfoIrProjector.
* @volume: A #UfoBuffer that describes the volume. It will be updated.
* @volume_roi: A #UfoRegion. It describes the ROI of the volume.
* @subset: A #UfoIrProjectionsSubset that describes a projections subset, which
* should be backprojected onto the @volume.
* @relax_param: A #gfloat is a relaxation parameter.
* @finish_event: A #gpointer to cl_event that defines the operation end.
*
* Performs forward-projection of the ROI region in the @volume and updates
* @measurements using the scaled result.
*/
void
ufo_ir_projector_BP_ROI (UfoIrProjector       *projector,
                         UfoBuffer            *volume,
                         UfoRegion            *volume_roi,
                         UfoBuffer            *measurements,
                         UfoIrProjectionsSubset *subset,
                         gfloat               relax_param,
                         gpointer             finish_event)
{
    g_return_if_fail (UFO_IR_IS_PROJECTOR (projector) &&
                      UFO_IS_BUFFER (volume) &&
                      volume_roi &&
                      UFO_IS_BUFFER (measurements) &&
                      subset);

    if (relax_param == 0.0f) {
      g_warning("%s : BP argument relax_param = 0. Operation skipped.",
                G_OBJECT_TYPE_NAME (projector));
    }

    UfoIrProjectorClass *klass = UFO_IR_PROJECTOR_GET_CLASS (projector);
    klass->BP_ROI(projector,
                  volume, volume_roi,
                  measurements, subset,
                  relax_param,
                  finish_event);
}

/**
* ufo_ir_projector_FP:
* @projector: A #UfoIrProjector.
* @volume: A #UfoBuffer that describes the volume.
* @measurements: A #UfoBuffer that describes the measurement. It will be updated.
* @subset: A #UfoIrProjectionsSubset describes a projections subset, which
* should be projected.
* @scale: A #gfloat is a scaling coefficient.
* @finish_event: A #gpointer to cl_event that defines the operation end.
*
* Performs forward-projection of the @volume and updates @measurements using
* the scaled result.
*/
void
ufo_ir_projector_FP (UfoIrProjector         *projector,
                     UfoBuffer              *volume,
                     UfoBuffer              *measurements,
                     UfoIrProjectionsSubset *subset,
                     gfloat                 scale,
                     gpointer               finish_event)
{
    UfoIrProjectorPrivate *priv = UFO_IR_PROJECTOR_GET_PRIVATE (projector);
    ufo_ir_projector_FP_ROI (projector,
                             volume,
                             &priv->full_volume_region,
                             measurements,
                             subset,
                             scale,
                             finish_event);
}

/**
* ufo_ir_projector_BP:
* @projector: A #UfoIrProjector.
* @volume: A #UfoBuffer that describes the volume. It will be updated.
* @subset: A #UfoIrProjectionsSubset that describes a projections subset, which
* should be backprojected onto the @volume.
* @relax_param: A #gfloat is a relaxation parameter.
* @finish_event: A #gpointer to cl_event that defines the operation end.
*
* Performs backprojection of the @measurements to the @volume.
*/
void
ufo_ir_projector_BP (UfoIrProjector         *projector,
                     UfoBuffer              *volume,
                     UfoBuffer              *measurements,
                     UfoIrProjectionsSubset *subset,
                     gfloat                 relax_param,
                     gpointer               finish_event)
{
    UfoIrProjectorPrivate *priv = UFO_IR_PROJECTOR_GET_PRIVATE (projector);
    ufo_ir_projector_BP_ROI (projector,
                             volume,
                             &priv->full_volume_region,
                             measurements,
                             subset,
                             relax_param,
                             finish_event);
}

static void
ufo_ir_projector_init (UfoIrProjector *self)
{
    UfoIrProjectorPrivate *priv = NULL;
    self->priv = priv = UFO_IR_PROJECTOR_GET_PRIVATE (self);
    priv->geometry = NULL;
}

/**
* ufo_ir_projector_from_json:
* @object: A #JsonObject object
* @manager: A #UfoPluginManager
*
* Loads a projection model module depending on Json object, initializes it and returns an instance.
*
* Returns: (transfer full): (allow-none): #gpointer or %NULL if module cannot be found
*/
gpointer
ufo_ir_projector_from_json (JsonObject       *object,
                            UfoPluginManager *manager)
{
    gboolean gpu = json_object_get_boolean_member (object, "on-gpu");
    const gchar *model = json_object_get_string_member (object, "model");
    gpointer plugin = NULL;

    if (gpu) {
        GError *tmp_error = NULL;
        plugin = ufo_plugin_manager_get_plugin (manager,
                                                "ufo_ir_cl_projector_new",
                                                "libufoir_cl_projector.so",
                                                &tmp_error);

        if (tmp_error != NULL) {
          g_warning ("%s", tmp_error->message);
          return NULL;
        }

        g_object_set (plugin, "model", model, NULL);
    }
    else {
      g_error ("Case for non-gpu projector is not implemented.");
    }

    return plugin;
}