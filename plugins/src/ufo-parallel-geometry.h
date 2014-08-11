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

#ifndef __UFO_PARALLEL_GEOMETRY_H
#define __UFO_PARALLEL_GEOMETRY_H

#include <ufo-geometry.h>

G_BEGIN_DECLS

#define UFO_TYPE_PARALLEL_GEOMETRY            (ufo_parallel_geometry_get_type())
#define UFO_PARALLEL_GEOMETRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_PARALLEL_GEOMETRY, UfoParallelGeometry))
#define UFO_IS_PARALLEL_GEOMETRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_PARALLEL_GEOMETRY))
#define UFO_PARALLEL_GEOMETRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_PARALLEL_GEOMETRY, UfoParallelGeometryClass))
#define UFO_IS_PARALLEL_GEOMETRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_PARALLEL_GEOMETRY))
#define UFO_PARALLEL_GEOMETRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_PARALLEL_GEOMETRY, UfoParallelGeometryClass))

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

typedef struct {
    float det_scale;
    float det_offset;
} UfoParallelGeometrySpec;

UfoGeometry *
ufo_parallel_geometry_new ();

GType ufo_parallel_geometry_get_type (void);

G_END_DECLS
#endif
