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

#ifndef __UFO_IR_ASDPOCS_TASK_H
#define __UFO_IR_ASDPOCS_TASK_H

#include "ufo-ir-method-task.h"

G_BEGIN_DECLS

#define UFO_IR_TYPE_ASDPOCS_TASK             (ufo_ir_asdpocs_task_get_type())
#define UFO_IR_ASDPOCS_TASK(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_ASDPOCS_TASK, UfoIrAsdpocsTask))
#define UFO_IR_IS_ASDPOCS_TASK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_ASDPOCS_TASK))
#define UFO_IR_ASDPOCS_TASK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_ASDPOCS_TASK, UfoIrAsdpocsTaskClass))
#define UFO_IR_IS_ASDPOCS_TASK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_ASDPOCS_TASK))
#define UFO_IR_ASDPOCS_TASK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_ASDPOCS_TASK, UfoIrAsdpocsTaskClass))

typedef struct _UfoIrAsdpocsTask           UfoIrAsdpocsTask;
typedef struct _UfoIrAsdpocsTaskClass      UfoIrAsdpocsTaskClass;
typedef struct _UfoIrAsdpocsTaskPrivate    UfoIrAsdpocsTaskPrivate;

struct _UfoIrAsdpocsTask {
    UfoIrMethodTask parent_instance;

    UfoIrAsdpocsTaskPrivate *priv;
};

struct _UfoIrAsdpocsTaskClass {
    UfoIrMethodTaskClass parent_class;
};

UfoNode  *ufo_ir_asdpocs_task_new       (void);
GType     ufo_ir_asdpocs_task_get_type  (void);

gfloat ufo_ir_asdpocs_task_get_beta(UfoIrAsdpocsTask *self);
void   ufo_ir_asdpocs_task_set_beta(UfoIrAsdpocsTask *self, gfloat value);

gfloat ufo_ir_asdpocs_task_get_beta_red(UfoIrAsdpocsTask *self);
void   ufo_ir_asdpocs_task_set_beta_red(UfoIrAsdpocsTask *self, gfloat value);

guint ufo_ir_asdpocs_task_get_ng(UfoIrAsdpocsTask *self);
void  ufo_ir_asdpocs_task_set_ng(UfoIrAsdpocsTask *self, guint value);

gfloat ufo_ir_asdpocs_task_get_alpha(UfoIrAsdpocsTask *self);
void   ufo_ir_asdpocs_task_set_alpha(UfoIrAsdpocsTask *self, gfloat value);

gfloat ufo_ir_asdpocs_task_get_alpha_red(UfoIrAsdpocsTask *self);
void   ufo_ir_asdpocs_task_set_alpha_red(UfoIrAsdpocsTask *self, gfloat value);

gfloat ufo_ir_asdpocs_task_get_r_max(UfoIrAsdpocsTask *self);
void   ufo_ir_asdpocs_task_set_r_max(UfoIrAsdpocsTask *self, gfloat value);

gboolean ufo_ir_asdpocs_task_get_positive_constraint(UfoIrAsdpocsTask *self);
void     ufo_ir_asdpocs_task_set_positive_constraint(UfoIrAsdpocsTask *self, gboolean value);
G_END_DECLS

#endif

