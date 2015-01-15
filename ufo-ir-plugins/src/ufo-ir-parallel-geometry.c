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

#include "ufo-ir-parallel-geometry.h"
#include <math.h>

#define BEAM_GEOMETRY "parallel"

static void ufo_copyable_interface_init (UfoCopyableIface *iface);
G_DEFINE_TYPE_WITH_CODE (UfoIrParallelGeometry, ufo_ir_parallel_geometry, UFO_IR_TYPE_GEOMETRY,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_COPYABLE,
                                                ufo_copyable_interface_init))

#define UFO_IR_PARALLEL_GEOMETRY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_PARALLEL_GEOMETRY, UfoIrParallelGeometryPrivate))

struct _UfoIrParallelGeometryPrivate {
    UfoIrParallelGeometrySpec spec;
};

enum {
    PROP_0 = N_IR_GEOMETRY_VIRTUAL_PROPERTIES,
    PROP_DETECTOR_SCALE,
    PROP_AXIS_POS,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoIrGeometry *
ufo_ir_parallel_geometry_new (void)
{
    return UFO_IR_GEOMETRY (g_object_new (UFO_IR_TYPE_PARALLEL_GEOMETRY, NULL));
}

static void
ufo_ir_parallel_geometry_set_property (GObject      *object,
                                       guint        property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
    UfoIrParallelGeometryPrivate *priv = UFO_IR_PARALLEL_GEOMETRY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_DETECTOR_SCALE:
            priv->spec.det_scale = g_value_get_float(value);
            break;
        case PROP_AXIS_POS:
            priv->spec.axis_pos = g_value_get_float(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_parallel_geometry_get_property (GObject    *object,
                                       guint      property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
    UfoIrParallelGeometryPrivate *priv = UFO_IR_PARALLEL_GEOMETRY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_BEAM_GEOMETRY:
            g_value_set_string (value, BEAM_GEOMETRY);
            break;
        case PROP_DETECTOR_SCALE:
            g_value_set_float (value, priv->spec.det_scale);
            break;
        case PROP_AXIS_POS:
            g_value_set_float (value, priv->spec.axis_pos);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}


static void
ufo_ir_parallel_geometry_configure_real (UfoIrGeometry  *geometry,
                                         UfoRequisition *input_req,
                                         GError         **error)
{
    UFO_IR_GEOMETRY_CLASS (ufo_ir_parallel_geometry_parent_class)->configure (geometry,
                                                                            input_req,
                                                                            error);
    if (error && *error)
      return;

    UfoIrParallelGeometryPrivate *priv = UFO_IR_PARALLEL_GEOMETRY_GET_PRIVATE (geometry);

    UfoIrGeometryDims *dims = NULL;
    gfloat det_scale = 1.0;
    g_object_get (geometry,
                  "dimensions", &dims,
                  "detector-scale", &det_scale,
                  NULL);

    dims->width = (unsigned long) ceil ((gfloat)dims->n_dets * det_scale);
    dims->height = (unsigned long) ceil ((gfloat)dims->n_dets * det_scale);

    if (input_req->n_dims == 3) {
        dims->depth = (unsigned long) ceil ((gfloat)dims->n_dets * det_scale);
    }

    if (priv->spec.axis_pos < 0) {
        priv->spec.axis_pos = (float)dims->n_dets / 2.0f;
    }
}

static void
ufo_ir_parallel_geometry_get_volume_requisitions_real (UfoIrGeometry  *geometry,
                                                       UfoRequisition *requisition)
{
    UfoIrGeometryDims *dims = NULL;
    g_object_get (geometry, "dimensions", &dims, NULL);

    if (dims->depth > 1)
        requisition->n_dims = 3;
    else
        requisition->n_dims = 2;

    requisition->dims[0] = dims->width;
    requisition->dims[1] = dims->height;
    requisition->dims[2] = dims->depth;
}

static gpointer
ufo_ir_parallel_geometry_get_spec_real (UfoIrGeometry *geometry,
                                        gsize         *data_size)
{
    UfoIrParallelGeometryPrivate *priv = UFO_IR_PARALLEL_GEOMETRY_GET_PRIVATE (geometry);
    *data_size = sizeof (UfoIrParallelGeometrySpec);
    return &priv->spec;
}

static UfoCopyable *
ufo_ir_parallel_geometry_copy_real (gpointer origin,
                                    gpointer _copy)
{
    UfoCopyable *copy;
    if (_copy)
        copy = UFO_COPYABLE(_copy);
    else
        copy = UFO_COPYABLE (ufo_ir_parallel_geometry_new());

    UfoIrParallelGeometryPrivate *priv = UFO_IR_PARALLEL_GEOMETRY_GET_PRIVATE (origin);
    g_object_set (G_OBJECT(copy),
                  "detector-scale", priv->spec.det_scale,
                  "axis-pos", priv->spec.axis_pos,
                  NULL);
    return copy;
}

static void
ufo_copyable_interface_init (UfoCopyableIface *iface)
{
    iface->copy = ufo_ir_parallel_geometry_copy_real;
}

static void
ufo_ir_parallel_geometry_class_init (UfoIrParallelGeometryClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->set_property = ufo_ir_parallel_geometry_set_property;
    gobject_class->get_property = ufo_ir_parallel_geometry_get_property;

    g_object_class_override_property(gobject_class,
                                     PROP_BEAM_GEOMETRY,
                                     "beam-geometry");

    properties[PROP_DETECTOR_SCALE] =
        g_param_spec_float ("detector-scale",
                            "Aspect ratio of pixel size and detector size.",
                            "Aspect ratio of pixel size and detector size.",
                            0.1f, 4.0f, 1.0f,
                            G_PARAM_READWRITE);

    properties[PROP_AXIS_POS] =
        g_param_spec_float ("axis-pos",
                            "Position of rotation axis",
                            "Position of rotation axis",
                            -1.0, +8192.0, 0.0,
                            G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof(UfoIrParallelGeometryPrivate));

    UFO_IR_GEOMETRY_CLASS (klass)->get_volume_requisitions =
        ufo_ir_parallel_geometry_get_volume_requisitions_real;
    UFO_IR_GEOMETRY_CLASS (klass)->get_spec = ufo_ir_parallel_geometry_get_spec_real;
    UFO_IR_GEOMETRY_CLASS (klass)->configure = ufo_ir_parallel_geometry_configure_real;
}

static void
ufo_ir_parallel_geometry_init(UfoIrParallelGeometry *self)
{
    UfoIrParallelGeometryPrivate *priv = NULL;
    self->priv = priv = UFO_IR_PARALLEL_GEOMETRY_GET_PRIVATE(self);
    priv->spec.det_scale = 1.0f;
    priv->spec.axis_pos = -1.0f;
}