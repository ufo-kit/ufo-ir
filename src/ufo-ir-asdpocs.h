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

#ifndef __UFO_IR_ASDPOCS_H
#define __UFO_IR_ASDPOCS_H

#include <glib-object.h>
#include <ufo-ir-method.h>

G_BEGIN_DECLS

#define UFO_TYPE_IR_ASDPOCS              (ufo_ir_asdpocs_get_type())
#define UFO_IR_ASDPOCS(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_IR_ASDPOCS, UfoIrASDPOCS))
#define UFO_IS_IR_ASDPOCS(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_IR_ASDPOCS))
#define UFO_IR_ASDPOCS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_IR_ASDPOCS, UfoIrASDPOCSClass))
#define UFO_IS_IR_ASDPOCS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_IR_ASDPOCS)
#define UFO_IR_ASDPOCS_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_IR_ASDPOCS, UfoIrASDPOCSClass))

GQuark ufo_ir_sart_error_quark (void);
#define UFO_IR_ASDPOCS_ERROR           ufo_ir_asdpocs_error_quark()

typedef enum {
    UFO_IR_ASDPOCS_ERROR_SETUP
} UfoIrASDPOCSError;

typedef struct _UfoIrASDPOCS         UfoIrASDPOCS;
typedef struct _UfoIrASDPOCSClass    UfoIrASDPOCSClass;
typedef struct _UfoIrASDPOCSPrivate  UfoIrASDPOCSPrivate;

struct _UfoIrASDPOCS {
    UfoIrMethod parent_instance;
    UfoIrASDPOCSPrivate *priv;
};

struct _UfoIrASDPOCSClass {
    UfoIrMethodClass parent_class;
};

UfoIrMethod*
ufo_ir_asdpocs_new ();

GType ufo_ir_asdpocs_get_type (void);
G_END_DECLS
#endif
