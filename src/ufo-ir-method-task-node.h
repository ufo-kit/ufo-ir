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

#ifndef __UFO_IR_METHOD_TASK_NODE_H
#define __UFO_IR_METHOD_TASK_NODE_H

#include <ufo/ufo.h>

G_BEGIN_DECLS

#define UFO_IR_TYPE_METHOD_TASK_NODE             (ufo_ir_method_task_node_get_type())
#define UFO_IR_METHOD_TASK_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_METHOD_TASK_NODE, UfoIrMethodTaskNode))
#define UFO_IS_IR_METHOD_TASK_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_METHOD_TASK_NODE))
#define UFO_IR_METHOD_TASK_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_METHOD_TASK_NODE, UfoIrMethodTaskNodeClass))
#define UFO_IS_IR_METHOD_TASK_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_METHOD_TASK_NODE))
#define UFO_IR_METHOD_TASK_NODE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_METHOD_TASK_NODE, UfoIrMethodTaskNodeClass))

typedef struct _UfoIrMethodTaskNode           UfoIrMethodTaskNode;
typedef struct _UfoIrMethodTaskNodeClass      UfoIrMethodTaskNodeClass;
typedef struct _UfoIrMethodTaskNodePrivate    UfoIrMethodTaskNodePrivate;

struct _UfoIrMethodTaskNode {
    UfoTaskNode parent_instance;
    UfoIrMethodTaskNodePrivate *priv;
};

struct _UfoIrMethodTaskNodeClass {
    UfoTaskNodeClass parent_class;
};

UfoNode  *ufo_ir_method_task_node_new       (void);
GType     ufo_ir_method_task_node_get_type  (void);

G_END_DECLS

#endif

