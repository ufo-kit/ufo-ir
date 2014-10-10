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

#ifndef __UFO_IR_GEOMETRY_H
#define __UFO_IR_GEOMETRY_H

#if !defined (__UFO_IR_H_INSIDE__) && !defined (UFO_IR_COMPILATION)
#error "Only <ufo/ir/ufo-ir.h> can be included directly."
#endif

#include <glib.h>
#include <ufo/ufo.h>

G_BEGIN_DECLS

#define UFO_IR_TYPE_GEOMETRY            (ufo_ir_geometry_get_type())
#define UFO_IR_GEOMETRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_GEOMETRY, UfoIrGeometry))
#define UFO_IR_IS_GEOMETRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_GEOMETRY))
#define UFO_IR_GEOMETRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_GEOMETRY, UfoIrGeometryClass))
#define UFO_IR_IS_GEOMETRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_GEOMETRY))
#define UFO_IR_GEOMETRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_GEOMETRY, UfoIrGeometryClass))

GQuark ufo_ir_geometry_error_quark (void);
#define UFO_IR_GEOMETRY_ERROR          ufo_ir_geometry_error_quark()

typedef struct _UfoIrGeometry         UfoIrGeometry;
typedef struct _UfoIrGeometryClass    UfoIrGeometryClass;
typedef struct _UfoIrGeometryPrivate  UfoIrGeometryPrivate;

typedef enum {
    UFO_IR_GEOMETRY_ERROR_INPUT_DATA
} UfoIrGeometryError;

enum {
    GEOMETRY_PROP_0 = 0,
    PROP_BEAM_GEOMETRY,
    N_IR_GEOMETRY_VIRTUAL_PROPERTIES
};

/**
* UfoIrGeometry:
*
* A #UfoIrGeometry stores the geometry parameters and precalculated sin and
* cos values for the scan angles.
*/
struct _UfoIrGeometry {
    /*< private >*/
    GObject parent_instance;
    UfoIrGeometryPrivate *priv;
};

/**
* UfoIrGeometryClass:
*
* #UfoIrGeometryClass class
*/
struct _UfoIrGeometryClass {
    /*< private >*/
    GObjectClass parent_class;

    void (*configure)               (UfoIrGeometry  *geometry,
                                     UfoRequisition *requisition,
                                     GError         **error);

    void (*get_volume_requisitions) (UfoIrGeometry  *geometry,
                                     UfoRequisition *requisition);

    gpointer (*get_spec)            (UfoIrGeometry *geometry,
                                     gsize         *data_size);

    void (*setup)                   (UfoIrGeometry  *geometry,
                                     UfoResources   *resources,
                                     GError         **error);
};


/**
* UfoAnglesType:
* @SIN_VALUES: Precalculated sin values.
* @COS_VALUES: Precalculated cos values.
*
* Types of precalculated values. Required for OpenCL kernels optimization.
*/
typedef enum {
    SIN_VALUES,
    COS_VALUES
} UfoIrAnglesType;

typedef struct {
  unsigned long height;
  unsigned long width;
  unsigned long depth;

  unsigned long n_dets;
  unsigned long n_angles;
} UfoIrGeometryDims;

UfoIrGeometry * ufo_ir_geometry_new              (void);
gfloat * ufo_ir_geometry_scan_angles_host        (UfoIrGeometry   *geometry,
                                                  UfoIrAnglesType type);
gpointer ufo_ir_geometry_scan_angles_device      (UfoIrGeometry   *geometry,
                                                  UfoIrAnglesType type);
void     ufo_ir_geometry_configure               (UfoIrGeometry  *geometry,
                                                  UfoRequisition *requisition,
                                                  GError         **error);
void     ufo_ir_geometry_get_volume_requisitions (UfoIrGeometry  *geometry,
                                                  UfoRequisition *requisition);
gpointer ufo_ir_geometry_get_spec                (UfoIrGeometry  *geometry,
                                                  gsize          *data_size);
void     ufo_ir_geometry_setup                   (UfoIrGeometry *geometry,
                                                  UfoResources  *resources,
                                                  GError        **error);
gpointer ufo_ir_geometry_from_json               (JsonObject       *object,
                                                  UfoPluginManager *manager);
GType    ufo_ir_geometry_get_type                (void);

G_END_DECLS
#endif