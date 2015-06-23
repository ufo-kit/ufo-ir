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

#ifndef __UFO_IR_BASIC_OPS
#define __UFO_IR_BASIC_OPS

#include <glib-object.h>
#include <ufo/ufo.h>

G_BEGIN_DECLS

gpointer ufo_ir_op_set (UfoBuffer *arg,
                        gfloat     value,
                        gpointer   command_queue,
                        gpointer   kernel);
gpointer ufo_ir_op_set_generate_kernel(UfoResources *resources);

gpointer ufo_ir_op_inv (UfoBuffer *arg,
                        gpointer   command_queue,
                        gpointer   kernel);
gpointer ufo_ir_op_inv_generate_kernel(UfoResources *resources);

gpointer ufo_ir_op_mul (UfoBuffer *arg1,
                        UfoBuffer *arg2,
                        UfoBuffer *out,
                        gpointer   command_queue,
                        gpointer   kernel);
gpointer ufo_ir_op_mul_generate_kernel(UfoResources *resources);

gpointer ufo_ir_op_add (UfoBuffer *arg1,
                        UfoBuffer *arg2,
                        UfoBuffer *out,
                        gpointer   command_queue,
                        gpointer   kernel);
gpointer ufo_ir_op_add_generate_kernel(UfoResources *resources);

G_END_DECLS

#endif

