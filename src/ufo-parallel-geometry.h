#ifndef __UFO_PARALLEL_GEOMETRY_H
#define __UFO_PARALLEL_GEOMETRY_H

#include <glib.h>
#include <ufo/ufo.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <ufo-geometry.h>

G_BEGIN_DECLS

#define UFO_TYPE_PARALLEL_GEOMETRY            (ufo_parallel_geometry_get_type())
#define UFO_PARALLEL_GEOMETRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_PARALLEL_GEOMETRY, UfoParallelGeometry))
#define UFO_IS_PARALLEL_GEOMETRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_PARALLEL_GEOMETRY))
#define UFO_PARALLEL_GEOMETRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_PARALLEL_GEOMETRY, UfoParallelGeometryClass))
#define UFO_IS_PARALLEL_GEOMETRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_PARALLEL_GEOMETRY))
#define UFO_PARALLEL_GEOMETRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_PARALLEL_GEOMETRY, UfoParallelGeometryClass))

GQuark ufo_parallel_geometry_error_quark (void);
#define UFO_PARALLEL_GEOMETRY_ERROR          ufo_parallel_geometry_error_quark()

typedef struct _UfoParallelGeometry         UfoParallelGeometry;
typedef struct _UfoParallelGeometryClass    UfoParallelGeometryClass;
typedef struct _UfoParallelGeometryPrivate  UfoParallelGeometryPrivate;


struct _UfoParallelGeometry {
    UfoGeometry parent_instance;
    UfoParallelGeometryPrivate *priv;
};

struct _UfoParallelGeometryClass {
    UfoGeometryClass parent_class;
};

UfoGeometry *
ufo_parallel_geometry_new ();


GType ufo_parallel_geometry_get_type (void);
G_END_DECLS
#endif
