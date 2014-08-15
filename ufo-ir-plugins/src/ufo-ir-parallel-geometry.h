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

#ifndef __UFO_IR_PARALLEL_GEOMETRY_H
#define __UFO_IR_PARALLEL_GEOMETRY_H

#include <ufo/ir/ufo-ir-geometry.h>

G_BEGIN_DECLS

#define UFO_IR_TYPE_PARALLEL_GEOMETRY            (ufo_ir_parallel_geometry_get_type())
#define UFO_IR_PARALLEL_GEOMETRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_PARALLEL_GEOMETRY, UfoIrParallelGeometry))
#define UFO_IR_IS_PARALLEL_GEOMETRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_PARALLEL_GEOMETRY))
#define UFO_IR_PARALLEL_GEOMETRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_PARALLEL_GEOMETRY, UfoIrParallelGeometryClass))
#define UFO_IR_IS_PARALLEL_GEOMETRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_PARALLEL_GEOMETRY))
#define UFO_IR_PARALLEL_GEOMETRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_PARALLEL_GEOMETRY, UfoIrParallelGeometryClass))

typedef struct _UfoIrParallelGeometry         UfoIrParallelGeometry;
typedef struct _UfoIrParallelGeometryClass    UfoIrParallelGeometryClass;
typedef struct _UfoIrParallelGeometryPrivate  UfoIrParallelGeometryPrivate;

struct _UfoIrParallelGeometry {
    UfoIrGeometry parent_instance;
    UfoIrParallelGeometryPrivate *priv;
};

struct _UfoIrParallelGeometryClass {
    UfoIrGeometryClass parent_class;
};

typedef struct {
    float det_scale;
    float det_offset;
} UfoIrParallelGeometrySpec;

UfoIrGeometry   *ufo_ir_parallel_geometry_new       (void);
GType            ufo_ir_parallel_geometry_get_type  (void);

G_END_DECLS
#endif
