#ifndef __UFO_IR_GEOMETRY_H
#define __UFO_IR_GEOMETRY_H

#include <glib.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

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
    UFO_IR_GEOMETRY_ERROR_SETUP
} UfoIrGeometryError;

struct _UfoIrGeometry {
    GObject parent_instance;
    UfoIrGeometryPrivate *priv;
};

struct _UfoIrGeometryClass {
    GObjectClass parent_class;
};

UfoIrGeometry *
ufo_ir_geometry_new (UfoMetaData *meta_geometry);

cl_mem
ufo_ir_geometry_angles_host (UfoIrGeometry *geometry,
                             UfoIrAnglesType type);

gfloat *
ufo_ir_geometry_angles_device (UfoIrGeometry *geometry,
                               UfoIrAnglesType type);

gfloat *
ufo_ir_geometry_angle (UfoIrGeometry *geometry,
                       guint          index,
                       float         *sin_val,
                       float         *cos_val);


GType ufo_ir_geometry_get_type (void);

G_END_DECLS
#endif
