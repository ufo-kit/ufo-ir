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

#ifndef __UFO_GEOMETRY_H
#define __UFO_GEOMETRY_H

#include <glib.h>
#include <ufo/ufo.h>

G_BEGIN_DECLS

#define UFO_TYPE_GEOMETRY            (ufo_geometry_get_type())
#define UFO_GEOMETRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_GEOMETRY, UfoGeometry))
#define UFO_IS_GEOMETRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_GEOMETRY))
#define UFO_GEOMETRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_GEOMETRY, UfoGeometryClass))
#define UFO_IS_GEOMETRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_GEOMETRY))
#define UFO_GEOMETRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_GEOMETRY, UfoGeometryClass))

GQuark ufo_geometry_error_quark (void);
#define UFO_GEOMETRY_ERROR          ufo_geometry_error_quark()

typedef struct _UfoGeometry         UfoGeometry;
typedef struct _UfoGeometryClass    UfoGeometryClass;
typedef struct _UfoGeometryPrivate  UfoGeometryPrivate;

typedef enum {
    UFO_GEOMETRY_ERROR_INPUT_DATA
} UfoGeometryError;

enum {
    GEOMETRY_PROP_0 = 0,
    PROP_BEAM_GEOMETRY,
    N_GEOMETRY_VIRTUAL_PROPERTIES
};

struct _UfoGeometry {
    GObject parent_instance;
    UfoGeometryPrivate *priv;
};

struct _UfoGeometryClass {
    GObjectClass parent_class;

    void (*configure) (UfoGeometry    *geometry,
                       UfoRequisition *requisition,
                       GError         **error);

    void (*get_volume_requisitions) (UfoGeometry    *geometry,
                                     UfoRequisition *requisition);

    gpointer (*get_spec) (UfoGeometry *geometry,
                          gsize *data_size);

    void (*setup) (UfoGeometry  *geometry,
                   UfoResources *resources,
                   GError       **error);
};

typedef enum {
    SIN_VALUES,
    COS_VALUES
} UfoAnglesType;

typedef struct {
  unsigned long height;
  unsigned long width;
  unsigned long depth;

  unsigned long n_dets;
  unsigned long n_angles;
} UfoGeometryDims;

UfoGeometry *
ufo_geometry_new (void);

gfloat *
ufo_geometry_scan_angles_host (UfoGeometry   *geometry,
                               UfoAnglesType type);

gpointer
ufo_geometry_scan_angles_device (UfoGeometry   *geometry,
                                 UfoAnglesType type);
void
ufo_geometry_configure (UfoGeometry    *geometry,
                        UfoRequisition *input_req,
                        GError         **error);

void
ufo_geometry_get_volume_requisitions (UfoGeometry    *geometry,
                                      UfoRequisition *requisition);

gpointer
ufo_geometry_get_spec (UfoGeometry *geometry,
                       gsize       *data_size);

void
ufo_geometry_setup (UfoGeometry  *geometry,
                    UfoResources *resources,
                    GError       **error);

gpointer
ufo_geometry_from_json (JsonObject       *object,
                        UfoPluginManager *manager);


GType ufo_geometry_get_type (void);

G_END_DECLS
#endif