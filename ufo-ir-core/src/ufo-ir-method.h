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

#ifndef __UFO_IR_METHOD_H
#define __UFO_IR_METHOD_H

#if !defined (__UFO_IR_H_INSIDE__) && !defined (UFO_IR_COMPILATION)
#error "Only <ufo/ir/ufo-ir.h> can be included directly."
#endif

#include <glib-object.h>
#include <ufo/ufo.h>

G_BEGIN_DECLS

#define UFO_IR_TYPE_METHOD              (ufo_ir_method_get_type())
#define UFO_IR_METHOD(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_METHOD, UfoIrMethod))
#define UFO_IR_IS_METHOD(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_METHOD))
#define UFO_IR_METHOD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_METHOD, UfoIrMethodClass))
#define UFO_IR_IS_METHOD_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_METHOD)
#define UFO_IR_METHOD_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_METHOD, UfoIrMethodClass))

typedef struct _UfoIrMethod         UfoIrMethod;
typedef struct _UfoIrMethodClass    UfoIrMethodClass;
typedef struct _UfoIrMethodPrivate  UfoIrMethodPrivate;

/**
* UfoIrMethod:
*
* A #UfoIrMethod is a base class for an iterative reconstruciton method.
*/
struct _UfoIrMethod {
    /*< private >*/
    UfoProcessor parent_instance;
    UfoIrMethodPrivate *priv;
};

struct _UfoIrMethodClass {
    /*< private >*/
    UfoProcessorClass parent_class;
};

UfoMethod* ufo_ir_method_new      (void);
GType      ufo_ir_method_get_type (void);

G_END_DECLS
#endif
