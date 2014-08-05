#ifndef __UFO_GEOMETRY_H
#define __UFO_GEOMETRY_H

#include <glib.h>
#include <ufo/ufo.h>
#include <ufo-common-routines.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

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

typedef enum {
    SIN_VALUES,
    COS_VALUES
} UfoAnglesType;

enum {
    GEOMETRY_PROP_0 = 0,
    PROP_NUM_ANGLES,
    PROP_ANGLE_STEP,
    PROP_ANGLE_OFFSET,
    PROP_BEAM_GEOMETRY,
    N_GEOMETRY_BASE_PROPERTIES
};

struct _UfoGeometry {
    GObject parent_instance;
    UfoGeometryPrivate *priv;
};

struct _UfoGeometryClass {
    GObjectClass parent_class;

    void (*get_volume_requisitions) (UfoGeometry    *geometry,
                                     UfoBuffer      *measurements,
                                     UfoRequisition *requisition,
                                     GError         **error);

    gsize (*get_meta) (UfoGeometry *geometry,
                       gpointer    *meta);

    void (*setup) (UfoGeometry  *geometry,
                   UfoResources *resources,
                   GError       **error);
};

UfoGeometry *
ufo_geometry_new ();

gfloat *
ufo_geometry_scan_angles_host (UfoGeometry *geometry,
                               UfoAnglesType type);

gpointer
ufo_geometry_scan_angles_device (UfoGeometry *geometry,
                                 UfoAnglesType type);

void
ufo_geometry_get_volume_requisitions (UfoGeometry    *geometry,
                                      UfoBuffer      *measurements,
                                      UfoRequisition *requisition,
                                      GError         **error);

gsize
ufo_geometry_get_meta (UfoGeometry *geometry,
                       gpointer    *meta);


void
ufo_geometry_setup (UfoGeometry  *geometry,
                    UfoResources *resources,
                    GError       **error);

gpointer
ufo_geometry_from_json (JsonObject       *object,
                        UfoPluginManager *manager,
                        GError           **error);


GType ufo_geometry_get_type (void);

G_END_DECLS
#endif
