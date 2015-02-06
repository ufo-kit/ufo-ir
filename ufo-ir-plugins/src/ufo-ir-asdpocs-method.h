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

#ifndef __UFO_IR_ASDPOCS_METHOD_H
#define __UFO_IR_ASDPOCS_METHOD_H

#include <glib-object.h>
#include <ufo/ir/ufo-ir.h>

G_BEGIN_DECLS

#define UFO_IR_TYPE_ASDPOCS_METHOD              (ufo_ir_asdpocs_method_get_type())
#define UFO_IR_ASDPOCS_METHOD(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_ASDPOCS_METHOD, UfoIrAsdPocsMethod))
#define UFO_IR_IS_ASDPOCS_METHOD(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_ASDPOCS_METHOD))
#define UFO_IR_ASDPOCS_METHOD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_ASDPOCS_METHOD, UfoIrAsdPocsMethodClass))
#define UFO_IR_IS_ASDPOCS_METHOD_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_ASDPOCS_METHOD)
#define UFO_IR_ASDPOCS_METHOD_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_ASDPOCS_METHOD, UfoIrAsdPocsMethodClass))

GQuark ufo_ir_asdpocs_method_error_quark (void);
#define UFO_IR_ASDPOCS_METHOD_ERROR ufo_ir_asdpocs_method_error_quark()

typedef enum {
    UFO_IR_ASDPOCS_METHOD_ERROR_SETUP
} UfoIrAsdPocsMethodError;

typedef struct _UfoIrAsdPocsMethod         UfoIrAsdPocsMethod;
typedef struct _UfoIrAsdPocsMethodClass    UfoIrAsdPocsMethodClass;
typedef struct _UfoIrAsdPocsMethodPrivate  UfoIrAsdPocsMethodPrivate;

struct _UfoIrAsdPocsMethod {
    UfoIrMethod parent_instance;
    UfoIrAsdPocsMethodPrivate *priv;
};

struct _UfoIrAsdPocsMethodClass {
    UfoIrMethodClass parent_class;
};

UfoIrMethod    *ufo_ir_asdpocs_method_new       (void);
void          ufo_ir_asdpocs_method_set_minimizer (UfoIrAsdPocsMethod *method,
                                                   UfoIrMethod *minimizer);
UfoIrMethod * ufo_ir_asdpocs_method_get_minimizer (UfoIrAsdPocsMethod *method);
GType           ufo_ir_asdpocs_method_get_type  (void);
G_END_DECLS
#endif
