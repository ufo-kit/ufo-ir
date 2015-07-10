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

#ifndef __UFO_IR_SART_TASK_H
#define __UFO_IR_SART_TASK_H

#include "core/ufo-ir-method-task.h"


G_BEGIN_DECLS

#define UFO_IR_TYPE_SART_TASK             (ufo_ir_sart_task_get_type())
#define UFO_IR_SART_TASK(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_SART_TASK, UfoIrSartTask))
#define UFO_IR_IS_SART_TASK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_SART_TASK))
#define UFO_IR_SART_TASK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_SART_TASK, UfoIrSartTaskClass))
#define UFO_IR_IS_SART_TASK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_SART_TASK))
#define UFO_IR_SART_TASK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_SART_TASK, UfoIrSartTaskClass))

typedef struct _UfoIrSartTask           UfoIrSartTask;
typedef struct _UfoIrSartTaskClass      UfoIrSartTaskClass;
typedef struct _UfoIrSartTaskPrivate    UfoIrSartTaskPrivate;

struct _UfoIrSartTask {
    UfoIrMethodTask parent_instance;

    UfoIrSartTaskPrivate *priv;
};

struct _UfoIrSartTaskClass {
    UfoIrMethodTaskClass parent_class;
};

UfoNode  *ufo_ir_sart_task_new       (void);
GType     ufo_ir_sart_task_get_type  (void);

gfloat ufo_ir_sart_task_get_relaxation_factor(UfoIrSartTask *self);
void   ufo_ir_sart_task_set_relaxation_factor(UfoIrSartTask *self, gfloat value);

G_END_DECLS

#endif

