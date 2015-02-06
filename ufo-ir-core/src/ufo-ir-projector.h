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

#ifndef __UFO_IR_PROJECTOR_H
#define __UFO_IR_PROJECTOR_H

#if !defined (__UFO_IR_H_INSIDE__) && !defined (UFO_IR_COMPILATION)
#error "Only <ufo/ir/ufo-ir.h> can be included directly."
#endif

#include <glib.h>
#include <ufo/ufo.h>

G_BEGIN_DECLS

#define UFO_IR_TYPE_PROJECTOR            (ufo_ir_projector_get_type())
#define UFO_IR_PROJECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_PROJECTOR, UfoIrProjector))
#define UFO_IR_IS_PROJECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_PROJECTOR))
#define UFO_IR_PROJECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_PROJECTOR, UfoIrProjectorClass))
#define UFO_IR_IS_PROJECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_PROJECTOR))
#define UFO_IR_PROJECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_PROJECTOR, UfoIrProjectorClass))

GQuark  ufo_ir_projector_error_quark    (void);
#define UFO_IR_PROJECTOR_ERROR          ufo_ir_projector_error_quark()

typedef struct _UfoIrProjector         UfoIrProjector;
typedef struct _UfoIrProjectorClass    UfoIrProjectorClass;
typedef struct _UfoIrProjectorPrivate  UfoIrProjectorPrivate;
typedef struct _UfoIrProjectionsSubset UfoIrProjectionsSubset;

typedef enum {
    UFO_IR_PROJECTOR_ERROR_SETUP
} UfoIrProjectorError;

/**
* UfoIrProjector:
*
* A #UfoIrProjector represents a base class for projection models implementation.
*/
struct _UfoIrProjector {
    /*< private >*/
    UfoProcessor parent_instance;
    UfoIrProjectorPrivate *priv;
};

/**
* UfoIrProjectorClass:
*
* #UfoIrProjectorClass class
*/
struct _UfoIrProjectorClass {
    /*< private >*/
    UfoProcessorClass parent_class;

    void (*FP_ROI) (UfoIrProjector         *projector,
                    UfoBuffer              *volume,
                    UfoRegion              *volume_roi,
                    UfoBuffer              *measurements,
                    UfoIrProjectionsSubset *subset,
                    gfloat                 scale,
                    gpointer               finish_event);

    void (*BP_ROI) (UfoIrProjector         *projector,
                    UfoBuffer              *volume,
                    UfoRegion              *volume_roi,
                    UfoBuffer              *measurements,
                    UfoIrProjectionsSubset *subset,
                    gfloat                 relax_param,
                    gpointer               finish_event);
};

/**
* UfoIrProjectionDirection:
* @Horizontal: Horizontal direction (angles from 45 deg to 135, etc.)
* @Vertical: Horizontal direction (angles from 0 deg to 45, from 135 to 315 etc.)
*
* Required for selecting an appropriate kernel or function.
*/
typedef enum {
    Horizontal = 0,
    Vertical
} UfoIrProjectionDirection;

/**
* UfoIrProjectionsSubset:
* @offset: Offset in number of projections
* @n: Number of projections in subset
* @direction: The direction of the projections in subset
* #UfoIrProjectionsSubset structure describing a set of projections in sinogram
*/
struct _UfoIrProjectionsSubset {
    guint offset;
    guint n;
    UfoIrProjectionDirection direction;
};

UfoIrProjector *ufo_ir_projector_new (void);
void      ufo_ir_projector_set_geometry (UfoIrProjector *projector,
                                         gpointer      geometry);
gpointer  ufo_ir_projector_get_geometry (UfoIrProjector *projector);

void      ufo_ir_projector_FP_ROI     (UfoIrProjector         *projector,
                                       UfoBuffer              *volume,
                                       UfoRegion              *volume_roi,
                                       UfoBuffer              *measurements,
                                       UfoIrProjectionsSubset *subset,
                                       gfloat                 scale,
                                       gpointer               finish_event);
void      ufo_ir_projector_BP_ROI     (UfoIrProjector         *projector,
                                       UfoBuffer              *volume,
                                       UfoRegion              *volume_roi,
                                       UfoBuffer              *measurements,
                                       UfoIrProjectionsSubset *subset,
                                       gfloat                 relax_param,
                                       gpointer               finish_event);
void      ufo_ir_projector_FP         (UfoIrProjector         *projector,
                                       UfoBuffer              *volume,
                                       UfoBuffer              *measurements,
                                       UfoIrProjectionsSubset *subset,
                                       gfloat                 scale,
                                       gpointer               finish_event);
void      ufo_ir_projector_BP         (UfoIrProjector         *projector,
                                       UfoBuffer              *volume,
                                       UfoBuffer              *measurements,
                                       UfoIrProjectionsSubset *subset,
                                       gfloat                 relax_param,
                                       gpointer               finish_event);
gpointer  ufo_ir_projector_from_json  (JsonObject       *object,
                                       UfoPluginManager *manager);
GType     ufo_ir_projector_get_type   (void);
G_END_DECLS
#endif
