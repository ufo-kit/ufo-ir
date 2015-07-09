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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __UFO_IR_PARALLEL_PROJECTOR_TASK_H
#define __UFO_IR_PARALLEL_PROJECTOR_TASK_H

#include "ufo-ir-projector-task.h"
#include <ufo/ufo.h>

G_BEGIN_DECLS

typedef struct _UfoIrProjectionsSubset UfoIrProjectionsSubset;

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

typedef struct {
  unsigned long height;
  unsigned long width;

  unsigned long n_dets;
  unsigned long n_angles;
} UfoIrGeometryDims;

#define UFO_IR_TYPE_PARALLEL_PROJECTOR_TASK             (ufo_ir_parallel_projector_task_get_type())
#define UFO_IR_PARALLEL_PROJECTOR_TASK(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_PARALLEL_PROJECTOR_TASK, UfoIrParallelProjectorTask))
#define UFO_IR_IS_PARALLEL_PROJECTOR_TASK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_PARALLEL_PROJECTOR_TASK))
#define UFO_IR_PARALLEL_PROJECTOR_TASK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_PARALLEL_PROJECTOR_TASK, UfoIrParallelProjectorTaskClass))
#define UFO_IR_IS_PARALLEL_PROJECTOR_TASK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_PARALLEL_PROJECTOR_TASK))
#define UFO_IR_PARALLEL_PROJECTOR_TASK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_PARALLEL_PROJECTOR_TASK, UfoIrParallelProjectorTaskClass))

typedef struct _UfoIrParallelProjectorTask           UfoIrParallelProjectorTask;
typedef struct _UfoIrParallelProjectorTaskClass      UfoIrParallelProjectorTaskClass;
typedef struct _UfoIrParallelProjectorTaskPrivate    UfoIrParallelProjectorTaskPrivate;

struct _UfoIrParallelProjectorTask {
    UfoIrProjectorTask parent_instance;

    UfoIrParallelProjectorTaskPrivate *priv;
};

struct _UfoIrParallelProjectorTaskClass {
    UfoIrProjectorTaskClass parent_class;
};

UfoNode  *ufo_ir_parallel_projector_task_new       (void);
GType     ufo_ir_parallel_projector_task_get_type  (void);

const gchar *ufo_ir_parallel_projector_get_model(UfoIrParallelProjectorTask *self);
void         ufo_ir_parallel_projector_set_model(UfoIrParallelProjectorTask *self, const gchar *model_name);

guint ufo_ir_parallel_projector_get_angles_num(UfoIrParallelProjectorTask *self);
void  ufo_ir_parallel_projector_set_angles_num(UfoIrParallelProjectorTask *self, guint angles_num);

void ufo_ir_parallel_projector_subset_fp(UfoIrParallelProjectorTask *self, UfoBuffer *volume, UfoBuffer *sinogram, UfoIrProjectionsSubset *subset);
void ufo_ir_parallel_projector_subset_bp(UfoIrParallelProjectorTask *self, UfoBuffer *volume, UfoBuffer *sinogram, UfoIrProjectionsSubset *subset);

const gfloat *ufo_ir_parallel_projector_get_host_sin_vals(UfoIrParallelProjectorTask *self);
const gfloat *ufo_ir_parallel_projector_get_host_cos_vals(UfoIrParallelProjectorTask *self);

G_END_DECLS

#endif
