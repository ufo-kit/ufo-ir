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

#ifndef __UFO_IR_SBTV_METHOD_H
#define __UFO_IR_SBTV_METHOD_H

#include <glib-object.h>
#include <ufo/ir/ufo-ir.h>

G_BEGIN_DECLS

#define UFO_IR_TYPE_SBTV_METHOD              (ufo_ir_sbtv_method_get_type())
#define UFO_IR_SBTV_METHOD(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_SBTV_METHOD, UfoIrSbtvMethod))
#define UFO_IR_IS_SBTV_METHOD(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_SBTV_METHOD))
#define UFO_IR_SBTV_METHOD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_SBTV_METHOD, UfoIrSbtvMethodClass))
#define UFO_IR_IS_SBTV_METHOD_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_SBTV_METHOD)
#define UFO_IR_SBTV_METHOD_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_SBTV_METHOD, UfoIrSbtvMethodClass))

GQuark ufo_ir_sbtv_method_error_quark (void);
#define UFO_IR_SBTV_METHOD_ERROR           ufo_ir_sbtv_method_error_quark()

typedef struct _UfoIrSbtvMethod         UfoIrSbtvMethod;
typedef struct _UfoIrSbtvMethodClass    UfoIrSbtvMethodClass;
typedef struct _UfoIrSbtvMethodPrivate  UfoIrSbtvMethodPrivate;

struct _UfoIrSbtvMethod {
    UfoIrMethod parent_instance;
    UfoIrSbtvMethodPrivate *priv;
};

struct _UfoIrSbtvMethodClass {
    UfoIrMethodClass parent_class;
};

UfoIrMethod *    ufo_ir_sbtv_method_new      (void);
GType            ufo_ir_sbtv_method_get_type (void);

G_END_DECLS
#endif
