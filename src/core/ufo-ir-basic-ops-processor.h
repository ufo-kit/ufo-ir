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

#ifndef __UFO_IR_BASIC_OPS_PROCESSOR_H
#define __UFO_IR_BASIC_OPS_PROCESSOR_H

#include <glib-object.h>
#include <ufo/ufo.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

G_BEGIN_DECLS

#define UFO_IR_TYPE_BASIC_OPS_PROCESSOR             (ufo_ir_basic_ops_processor_get_type())
#define UFO_IR_BASIC_OPS_PROCESSOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_BASIC_OPS_PROCESSOR, UfoIrBasicOpsProcessor))
#define UFO_IR_IS_BASIC_OPS_PROCESSOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_BASIC_OPS_PROCESSOR))
#define UFO_IR_BASIC_OPS_PROCESSOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_BASIC_OPS_PROCESSOR, UfoIrBasicOpsProcessorClass))
#define UFO_IR_IS_BASIC_OPS_PROCESSOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_BASIC_OPS_PROCESSOR))
#define UFO_IR_BASIC_OPS_PROCESSOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_BASIC_OPS_PROCESSOR, UfoIrBasicOpsProcessorClass))

typedef struct _UfoIrBasicOpsProcessor           UfoIrBasicOpsProcessor;
typedef struct _UfoIrBasicOpsProcessorClass      UfoIrBasicOpsProcessorClass;
typedef struct _UfoIrBasicOpsProcessorPrivate    UfoIrBasicOpsProcessorPrivate;

struct _UfoIrBasicOpsProcessor {
    GObject parent_instance;

    UfoIrBasicOpsProcessorPrivate *priv;
};

struct _UfoIrBasicOpsProcessorClass {
    GObjectClass parent_class;
};

UfoIrBasicOpsProcessor *ufo_ir_basic_ops_processor_new (UfoResources *resources, cl_command_queue cmd_queue);
GType     ufo_ir_basic_ops_processor_get_type (void);

gpointer ufo_ir_basic_ops_processor_add (UfoIrBasicOpsProcessor *self, UfoBuffer *buffer1, UfoBuffer *buffer2, UfoBuffer *result);
gpointer ufo_ir_basic_ops_processor_add2 (UfoIrBasicOpsProcessor *self, UfoBuffer *buffer1, UfoBuffer *buffer2, gfloat modifier, UfoBuffer *result);
gpointer ufo_ir_basic_ops_processor_deduction (UfoIrBasicOpsProcessor *self, UfoBuffer *buffer1, UfoBuffer *buffer2, UfoBuffer *result);
gpointer ufo_ir_basic_ops_processor_deduction2 (UfoIrBasicOpsProcessor *self, UfoBuffer *buffer1, UfoBuffer *buffer2, gfloat modifier, UfoBuffer *result);
void     ufo_ir_basic_ops_processor_div_element_wise(UfoIrBasicOpsProcessor *self, UfoBuffer *buffer1, UfoBuffer *buffer2, UfoBuffer *result);
gfloat   ufo_ir_basic_ops_processor_dot_product(UfoIrBasicOpsProcessor *self, UfoBuffer *buffer1, UfoBuffer *buffer2);
gpointer ufo_ir_basic_ops_processor_inv (UfoIrBasicOpsProcessor *self, UfoBuffer *buffer);
gfloat   ufo_ir_basic_ops_processor_l1_norm (UfoIrBasicOpsProcessor *self, UfoBuffer *buffer);
gfloat   ufo_ir_basic_ops_processor_l2_norm (UfoIrBasicOpsProcessor *self, UfoBuffer *buffer);
void     ufo_ir_basic_ops_processor_max_element_wise(UfoIrBasicOpsProcessor *self, UfoBuffer *buffer1, UfoBuffer *buffer2, UfoBuffer *result);
gpointer ufo_ir_basic_ops_processor_mul (UfoIrBasicOpsProcessor *self, UfoBuffer *buffer1, UfoBuffer *buffer2, UfoBuffer *result);
void     ufo_ir_basic_ops_processor_mul_element_wise(UfoIrBasicOpsProcessor *self, UfoBuffer *buffer1, UfoBuffer *buffer2, UfoBuffer *result);
gpointer ufo_ir_basic_ops_processor_mul_rows (UfoIrBasicOpsProcessor *self, UfoBuffer *buffer1, UfoBuffer *buffer2, UfoBuffer *result, guint offset, guint n);
void     ufo_ir_basic_ops_processor_mul_scalar(UfoIrBasicOpsProcessor *self, UfoBuffer *buffer, gfloat multiplier);
void     ufo_ir_basic_ops_processor_normalization(UfoIrBasicOpsProcessor *self, UfoBuffer *buffer);
gpointer ufo_ir_basic_ops_processor_positive_constraint (UfoIrBasicOpsProcessor *self, UfoBuffer *buffer, UfoBuffer *result);
gpointer ufo_ir_basic_ops_processor_set (UfoIrBasicOpsProcessor *self, UfoBuffer *buffer, gfloat value);
void     ufo_ir_basic_ops_processor_sqrt(UfoIrBasicOpsProcessor *self, UfoBuffer *buffer);

G_END_DECLS

#endif
