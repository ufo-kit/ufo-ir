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

#ifndef __UFO_IR_SBTV_TASK_H
#define __UFO_IR_SBTV_TASK_H

#include <ufo/ufo.h>
#include "core/ufo-ir-method-task.h"

G_BEGIN_DECLS

#define UFO_IR_TYPE_SBTV_TASK             (ufo_ir_sbtv_task_get_type())
#define UFO_IR_SBTV_TASK(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_SBTV_TASK, UfoIrSbtvTask))
#define UFO_IR_IS_SBTV_TASK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_SBTV_TASK))
#define UFO_IR_SBTV_TASK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_SBTV_TASK, UfoIrSbtvTaskClass))
#define UFO_IR_IS_SBTV_TASK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_SBTV_TASK))
#define UFO_IR_SBTV_TASK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_SBTV_TASK, UfoIrSbtvTaskClass))

typedef struct _UfoIrSbtvTask           UfoIrSbtvTask;
typedef struct _UfoIrSbtvTaskClass      UfoIrSbtvTaskClass;
typedef struct _UfoIrSbtvTaskPrivate    UfoIrSbtvTaskPrivate;

struct _UfoIrSbtvTask {
    UfoIrMethodTask parent_instance;

    UfoIrSbtvTaskPrivate *priv;
};

struct _UfoIrSbtvTaskClass {
    UfoIrMethodTaskClass parent_class;
};

UfoNode  *ufo_ir_sbtv_task_new       (void);
GType     ufo_ir_sbtv_task_get_type  (void);

gfloat ufo_ir_sbtv_task_get_mu(UfoIrSbtvTask *self);
void   ufo_ir_sbtv_task_set_mu(UfoIrSbtvTask *self, gfloat value);

gfloat ufo_ir_sbtv_task_get_lambda(UfoIrSbtvTask *self);
void   ufo_ir_sbtv_task_set_lambda(UfoIrSbtvTask *self, gfloat value);

G_END_DECLS

#endif
