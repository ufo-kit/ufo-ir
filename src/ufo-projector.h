#ifndef __UFO_PROJECTOR_H
#define __UFO_PROJECTOR_H

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <glib.h>
#include <ufo/ufo.h>

/*
  Geometry
    |- Model name
    |- Parallel/conical
   ------
  /      \
par       conic
@1        @1
@2        @2
          @3

add function to Geometru
ufo_geometry_get_parameters (geometry, gsize *parameters_size, gpointer *p_parameters);


  projector-parallel-joseph.cl
    |- FP_horizontal
    |- FP_vertical
    |- BP

  projector-conical-dd.cl
    |- FP_horizontal
    |- FP_vertical
    |- BP
*/

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
                    UfoProjectionsSubset subset,
                    gfloat               scale,
                    cl_event             *finish_event);

    void (*BP_ROI) (UfoProjector         *projector,
                    UfoBuffer            *volume,
                    UfoRegion            *volume_roi,
                    UfoBuffer            *measurements,
                    UfoProjectionsSubset subset,
                    cl_event             *finish_event);

    void (*setup) (UfoProjector  *projector,
                   UfoResources  *resources,
                   GError        **error);
};

typedef enum {
    Vertical = 1,
    Horizontal = 0
} Direction;

struct _UfoProjectionsSubset {
  guint offset;
  guint n;
  Direction direction;
};

UfoProjector *
ufo_projector_new ();

void
ufo_projector_FP_ROI (UfoProjector         *projector,
                      UfoBuffer            *volume,
                      UfoRegion            *volume_roi,
                      UfoBuffer            *measurements,
                      UfoProjectionsSubset subset,
                      gfloat               scale,
                      cl_event             *finish_event);

void
ufo_projector_BP_ROI (UfoProjector         *projector,
                      UfoBuffer            *volume,
                      UfoRegion            *volume_roi,
                      UfoBuffer            *measurements,
                      UfoProjectionsSubset subset,
                      cl_event             *finish_event);

void
ufo_projector_FP (UfoProjector         *projector,
                  UfoBuffer            *volume,
                  UfoBuffer            *measurements,
                  UfoProjectionsSubset subset,
                  gfloat               scale,
                  cl_event             *finish_event);

void
ufo_projector_BP (UfoProjector         *projector,
                  UfoBuffer            *volume,
                  UfoBuffer            *measurements,
                  UfoProjectionsSubset subset,
                  cl_event             *finish_event);

void
ufo_projector_setup (UfoProjector *projector,
                     UfoResources *resources,
                     GError       **error);

gpointer
ufo_projector_from_json (JsonObject       *object,
                         UfoPluginManager *manager,
                         GError           **error);

GType ufo_projector_get_type (void);
G_END_DECLS
#endif
