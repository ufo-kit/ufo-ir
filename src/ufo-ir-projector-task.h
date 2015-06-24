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

#ifndef __UFO_IR_PROJECTOR_TASK_H
#define __UFO_IR_PROJECTOR_TASK_H

#include "ufo-ir-state-dependent-task.h"

G_BEGIN_DECLS

#define UFO_IR_TYPE_PROJECTOR_TASK             (ufo_ir_projector_task_get_type())
#define UFO_IR_PROJECTOR_TASK(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_PROJECTOR_TASK, UfoIrProjectorTask))
#define UFO_IR_IS_PROJECTOR_TASK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_PROJECTOR_TASK))
#define UFO_IR_PROJECTOR_TASK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_PROJECTOR_TASK, UfoIrProjectorTaskClass))
#define UFO_IR_IS_PROJECTOR_TASK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_PROJECTOR_TASK))
#define UFO_IR_PROJECTOR_TASK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_PROJECTOR_TASK, UfoIrProjectorTaskClass))

typedef struct _UfoIrProjectorTask           UfoIrProjectorTask;
typedef struct _UfoIrProjectorTaskClass      UfoIrProjectorTaskClass;
typedef struct _UfoIrProjectorTaskPrivate    UfoIrProjectorTaskPrivate;

struct _UfoIrProjectorTask {
    UfoIrStateDependentTask parent_instance;

    UfoIrProjectorTaskPrivate *priv;
};

struct _UfoIrProjectorTaskClass {
    UfoIrStateDependentTaskClass parent_class;
};

UfoNode  *ufo_ir_projector_task_new       (void);
GType     ufo_ir_projector_task_get_type  (void);

gfloat ufo_ir_projector_task_get_step(UfoIrProjectorTask *self);
void   ufo_ir_projector_task_set_step(UfoIrProjectorTask *self, gfloat value);

gfloat ufo_ir_projector_task_get_axis_position(UfoIrProjectorTask *self);
void   ufo_ir_projector_task_set_axis_position(UfoIrProjectorTask *self, gfloat value);

gfloat ufo_ir_projector_task_get_relaxation(UfoIrProjectorTask *self);
void   ufo_ir_projector_task_set_relaxation(UfoIrProjectorTask *self, gfloat value);

gfloat ufo_ir_projector_task_get_correction_scale(UfoIrProjectorTask *self);
void   ufo_ir_projector_task_set_correction_scale(UfoIrProjectorTask *self, gfloat value);

G_END_DECLS

#endif
