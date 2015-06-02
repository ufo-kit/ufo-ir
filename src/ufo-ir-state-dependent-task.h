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

#ifndef __UFO_IR_STATE_DEPENDENT_TASK_H
#define __UFO_IR_STATE_DEPENDENT_TASK_H

#include <ufo/ufo.h>

G_BEGIN_DECLS

#define UFO_TYPE_IR_STATE_DEPENDENT_TASK             (ufo_ir_state_dependent_task_get_type())
#define UFO_IR_STATE_DEPENDENT_TASK(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_IR_STATE_DEPENDENT_TASK, UfoIrStateDependentTask))
#define UFO_IR_IS_STATE_DEPENDENT_TASK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_IR_STATE_DEPENDENT_TASK))
#define UFO_IR_STATE_DEPENDENT_TASK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_IR_STATE_DEPENDENT_TASK, UfoIrStateDependentTaskClass))
#define UFO_IR_IS_STATE_DEPENDENT_TASK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_IR_STATE_DEPENDENT_TASK))
#define UFO_IR_STATE_DEPENDENT_TASK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_IR_STATE_DEPENDENT_TASK, UfoIrStateDependentTaskClass))

typedef struct _UfoIrStateDependentTask           UfoIrStateDependentTask;
typedef struct _UfoIrStateDependentTaskClass      UfoIrStateDependentTaskClass;
typedef struct _UfoIrStateDependentTaskPrivate    UfoIrStateDependentTaskPrivate;

struct _UfoIrStateDependentTask {
    UfoTaskNode parent_instance;

    UfoIrStateDependentTaskPrivate *priv;
};

struct _UfoIrStateDependentTaskClass {
    UfoTaskNodeClass parent_class;

    gboolean (*forward)(UfoTask        *task,
                        UfoBuffer     **inputs,
                        UfoBuffer      *output,
                        UfoRequisition *requisition);

    gboolean (*backward)(UfoTask        *task,
                         UfoBuffer     **inputs,
                         UfoBuffer      *output,
                         UfoRequisition *requisition);
};

UfoNode  *ufo_ir_state_dependent_task_new       (void);
GType     ufo_ir_state_dependent_task_get_type  (void);

gboolean ufo_ir_state_dependent_task_forward (UfoTask        *task,
                                              UfoBuffer     **inputs,
                                              UfoBuffer      *output,
                                              UfoRequisition *requisition);

gboolean ufo_ir_state_dependent_task_backward (UfoTask        *task,
                                               UfoBuffer     **inputs,
                                               UfoBuffer      *output,
                                               UfoRequisition *requisition);
G_END_DECLS

#endif

