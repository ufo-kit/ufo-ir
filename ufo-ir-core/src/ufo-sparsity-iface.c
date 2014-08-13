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

#include "ufo-sparsity-iface.h"

typedef UfoSparsityIface UfoSparsityInterface;

G_DEFINE_INTERFACE (UfoSparsity, ufo_sparsity, G_TYPE_OBJECT)

gboolean
ufo_sparsity_minimize (UfoSparsity *sparsity,
                       UfoBuffer *input,
                       UfoBuffer *output)
{
    g_return_val_if_fail(UFO_IS_SPARSITY (sparsity) &&
                         UFO_IS_BUFFER (input) &&
                         UFO_IS_BUFFER (output),
                         FALSE);
    return UFO_SPARSITY_GET_IFACE (sparsity)->minimize (sparsity, input, output);
}

static gboolean
ufo_sparsity_minimize_real (UfoSparsity *sparsity,
                            UfoBuffer *input,
                            UfoBuffer *output)
{
    g_warning ("%s: `minimize' not implemented", G_OBJECT_TYPE_NAME (sparsity));
    return FALSE;
}

static void
ufo_sparsity_default_init (UfoSparsityInterface *iface)
{
    iface->minimize = ufo_sparsity_minimize_real;
}