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

#ifndef __UFO_PROJECTOR_H
#define __UFO_PROJECTOR_H

#include <glib.h>
#include <ufo/ufo.h>

G_BEGIN_DECLS

#define UFO_TYPE_PROJECTOR            (ufo_projector_get_type())
#define UFO_PROJECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_PROJECTOR, UfoProjector))
#define UFO_IS_PROJECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_PROJECTOR))
#define UFO_PROJECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_PROJECTOR, UfoProjectorClass))
#define UFO_IS_PROJECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_PROJECTOR))
#define UFO_PROJECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_PROJECTOR, UfoProjectorClass))

GQuark ufo_projector_error_quark (void);
#define UFO_PROJECTOR_ERROR          ufo_projector_error_quark()

typedef struct _UfoProjector         UfoProjector;
typedef struct _UfoProjectorClass    UfoProjectorClass;
typedef struct _UfoProjectorPrivate  UfoProjectorPrivate;
typedef struct _UfoProjectionsSubset UfoProjectionsSubset;

typedef enum {
    UFO_PROJECTOR_ERROR_SETUP
} UfoProjectorError;

struct _UfoProjector {
    GObject parent_instance;
    UfoProjectorPrivate *priv;
};

struct _UfoProjectorClass {
    GObjectClass parent_class;

    void (*FP_ROI) (UfoProjector         *projector,
                    UfoBuffer            *volume,
                    UfoRegion            *volume_roi,
                    UfoBuffer            *measurements,
                    UfoProjectionsSubset *subset,
                    gfloat               scale,
                    gpointer             finish_event);

    void (*BP_ROI) (UfoProjector         *projector,
                    UfoBuffer            *volume,
                    UfoRegion            *volume_roi,
                    UfoBuffer            *measurements,
                    UfoProjectionsSubset *subset,
                    gfloat               relax_param,
                    gpointer             finish_event);

    void (*setup) (UfoProjector  *projector,
                   UfoResources  *resources,
                   GError        **error);

    void (*configure) (UfoProjector  *projector);
};

typedef enum {
    Horizontal = 0,
    Vertical
} Direction;

struct _UfoProjectionsSubset {
  guint offset;
  guint n;
  Direction direction;
};

UfoProjector *
ufo_projector_new (void);

void
ufo_projector_FP_ROI (UfoProjector         *projector,
                      UfoBuffer            *volume,
                      UfoRegion            *volume_roi,
                      UfoBuffer            *measurements,
                      UfoProjectionsSubset *subset,
                      gfloat               scale,
                      gpointer             finish_event);

void
ufo_projector_BP_ROI (UfoProjector         *projector,
                      UfoBuffer            *volume,
                      UfoRegion            *volume_roi,
                      UfoBuffer            *measurements,
                      UfoProjectionsSubset *subset,
                      gfloat               relax_param,
                      gpointer             finish_event);

void
ufo_projector_FP (UfoProjector         *projector,
                  UfoBuffer            *volume,
                  UfoBuffer            *measurements,
                  UfoProjectionsSubset *subset,
                  gfloat               scale,
                  gpointer             finish_event);

void
ufo_projector_BP (UfoProjector         *projector,
                  UfoBuffer            *volume,
                  UfoBuffer            *measurements,
                  UfoProjectionsSubset *subset,
                  gfloat               relax_param,
                  gpointer             finish_event);

void
ufo_projector_setup (UfoProjector *projector,
                     UfoResources *resources,
                     GError       **error);

void
ufo_projector_configure (UfoProjector *projector);

gpointer
ufo_projector_from_json (JsonObject       *object,
                         UfoPluginManager *manager);

GType ufo_projector_get_type (void);
G_END_DECLS
#endif
