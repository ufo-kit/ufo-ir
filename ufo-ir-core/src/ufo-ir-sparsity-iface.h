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

#ifndef UFO_IR_SPARSITY_IFACE_H
#define UFO_IR_SPARSITY_IFACE_H

#if !defined (__UFO_IR_H_INSIDE__) && !defined (UFO_IR_COMPILATION)
#error "Only <ufo/ir/ufo-ir.h> can be included directly."
#endif

#include <glib-object.h>
#include <ufo/ufo.h>

G_BEGIN_DECLS

#define UFO_IR_TYPE_SPARSITY            (ufo_ir_sparsity_get_type())
#define UFO_IR_SPARSITY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_SPARSITY, UfoIrSparsity))
#define UFO_IR_SPARSITY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_SPARSITY, UfoIrSparsityIface))
#define UFO_IR_IS_SPARSITY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_SPARSITY))
#define UFO_IR_IS_SPARSITY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_SPARSITY))
#define UFO_IR_SPARSITY_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE((inst), UFO_IR_TYPE_SPARSITY, UfoIrSparsityIface))

typedef struct _UfoIrSparsity       UfoIrSparsity;
typedef struct _UfoIrSparsityIface  UfoIrSparsityIface;

struct _UfoIrSparsityIface {
    /*< private >*/
    GTypeInterface parent_iface;

    gboolean (*minimize) (UfoIrSparsity *sparsity,
                          UfoBuffer     *input,
                          UfoBuffer     *output,
                          gpointer      pevent);
};

gboolean ufo_ir_sparsity_minimize (UfoIrSparsity *sparsity,
                                   UfoBuffer     *input,
                                   UfoBuffer     *output,
                                   gpointer      pevent);
GType    ufo_ir_sparsity_get_type (void);

G_END_DECLS
#endif