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

#ifndef __UFO_IR_METHOD_TASK_H
#define __UFO_IR_METHOD_TASK_H

#include <ufo/ufo.h>
#include "ufo-ir-projector-task.h"

G_BEGIN_DECLS

#define UFO_IR_TYPE_METHOD_TASK             (ufo_ir_method_task_get_type())
#define UFO_IR_METHOD_TASK(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_METHOD_TASK, UfoIrMethodTask))
#define UFO_IR_IS_METHOD_TASK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_METHOD_TASK))
#define UFO_IR_METHOD_TASK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_METHOD_TASK, UfoIrMethodTaskClass))
#define UFO_IR_IS_METHOD_TASK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_METHOD_TASK))
#define UFO_IR_METHOD_TASK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_METHOD_TASK, UfoIrMethodTaskClass))

typedef struct _UfoIrMethodTask           UfoIrMethodTask;
typedef struct _UfoIrMethodTaskClass      UfoIrMethodTaskClass;
typedef struct _UfoIrMethodTaskPrivate    UfoIrMethodTaskPrivate;

struct _UfoIrMethodTask {
    UfoTaskNode parent_instance;
    UfoIrMethodTaskPrivate *priv;
};

struct _UfoIrMethodTaskClass {
    UfoTaskNodeClass parent_class;
};

UfoNode  *ufo_ir_method_task_new       (void);
GType     ufo_ir_method_task_get_type  (void);

UfoIrProjectorTask *ufo_ir_method_task_get_projector(UfoIrMethodTask *self);
void                ufo_ir_method_task_set_projector(UfoIrMethodTask *self, UfoIrProjectorTask *value);

guint ufo_ir_method_task_get_iterations_number(UfoIrMethodTask *self);
void  ufo_ir_method_task_set_iterations_number(UfoIrMethodTask *self, guint value);

G_END_DECLS

#endif

