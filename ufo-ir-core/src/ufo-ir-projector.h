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

#include <glib.h>
#include <ufo/ufo.h>
#include <ufo/methods/ufo-processor.h>

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

struct _UfoIrProjector {
    UfoProcessor parent_instance;
    UfoIrProjectorPrivate *priv;
};

struct _UfoIrProjectorClass {
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

typedef enum {
    Horizontal = 0,
    Vertical
} Direction;

struct _UfoIrProjectionsSubset {
    guint offset;
    guint n;
    Direction direction;
};

UfoIrProjector *ufo_ir_projector_new (void);
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
