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

#ifndef __UFO_IR_SIRT_METHOD_H
#define __UFO_IR_SIRT_METHOD_H

#include <glib-object.h>
#include <ufo/ir/ufo-ir.h>

G_BEGIN_DECLS

#define UFO_IR_TYPE_SIRT_METHOD              (ufo_ir_sirt_method_get_type())
#define UFO_IR_SIRT_METHOD(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_SIRT_METHOD, UfoIrSirtMethod))
#define UFO_IR_IS_SIRT_METHOD(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_SIRT_METHOD))
#define UFO_IR_SIRT_METHOD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_SIRT_METHOD, UfoIrSirtMethodClass))
#define UFO_IR_IS_SIRT_METHOD_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_SIRT_METHOD)
#define UFO_IR_SIRT_METHOD_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_SIRT_METHOD, UfoIrSirtMethodClass))

GQuark ufo_ir_sirt_method_error_quark (void);
#define UFO_IR_SIRT_METHOD_ERROR           ufo_ir_sirt_method_error_quark()

typedef struct _UfoIrSirtMethod         UfoIrSirtMethod;
typedef struct _UfoIrSirtMethodClass    UfoIrSirtMethodClass;

struct _UfoIrSirtMethod {
    UfoIrMethod parent_instance;
};

struct _UfoIrSirtMethodClass {
    UfoIrMethodClass parent_class;
};

UfoIrMethod *    ufo_ir_sirt_method_new      (void);
GType            ufo_ir_sirt_method_get_type (void);
G_END_DECLS
#endif
