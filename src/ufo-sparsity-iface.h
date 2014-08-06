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

#ifndef UFO_SPARSITY_IFACE_H
#define UFO_SPARSITY_IFACE_H

#include <glib-object.h>
#include <ufo/ufo.h>
#include <ufo-common-routines.h>

G_BEGIN_DECLS

#define UFO_TYPE_SPARSITY (ufo_sparsity_get_type())
#define UFO_SPARSITY(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_SPARSITY, UfoSparsity))
#define UFO_SPARSITY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_SPARSITY, UfoSparsityIface))
#define UFO_IS_SPARSITY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_SPARSITY))
#define UFO_IS_SPARSITY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_SPARSITY))
#define UFO_SPARSITY_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE((inst), UFO_TYPE_SPARSITY, UfoSparsityIface))

typedef struct _UfoSparsity UfoSparsity;
typedef struct _UfoSparsityIface UfoSparsityIface;

struct _UfoSparsityIface {
    /*< private >*/
    GTypeInterface parent_iface;

    gboolean (*minimize) (UfoSparsity *sparsity,
                          UfoBuffer   *input,
                          UfoBuffer   *output);
};

gboolean
ufo_sparsity_minimize (UfoSparsity *sparsity,
                       UfoBuffer   *input,
                       UfoBuffer   *output);

GType ufo_sparsity_get_type (void);

G_END_DECLS
#endif