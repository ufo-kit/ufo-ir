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

#ifndef __UFO_IR_GRADIENT_PROCESSOR_H
#define __UFO_IR_GRADIENT_PROCESSOR_H

#include <ufo/ufo.h>
#include <glib.h>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif


G_BEGIN_DECLS

#define UFO_IR_TYPE_GRADIENT_PROCESSOR             (ufo_ir_gradient_processor_get_type())
#define UFO_IR_GRADIENT_PROCESSOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_GRADIENT_PROCESSOR, UfoIrGradientProcessor))
#define UFO_IR_IS_GRADIENT_PROCESSOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_GRADIENT_PROCESSOR))
#define UFO_IR_GRADIENT_PROCESSOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_GRADIENT_PROCESSOR, UfoIrGradientProcessorClass))
#define UFO_IR_IS_GRADIENT_PROCESSOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_GRADIENT_PROCESSOR))
#define UFO_IR_GRADIENT_PROCESSOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_GRADIENT_PROCESSOR, UfoIrGradientProcessorClass))

typedef struct _UfoIrGradientProcessor           UfoIrGradientProcessor;
typedef struct _UfoIrGradientProcessorClass      UfoIrGradientProcessorClass;
typedef struct _UfoIrGradientProcessorPrivate    UfoIrGradientProcessorPrivate;

struct _UfoIrGradientProcessor {
    GObject parent_instance;

    UfoIrGradientProcessorPrivate *priv;
};

struct _UfoIrGradientProcessorClass {
    GObjectClass parent_class;
};

UfoIrGradientProcessor *ufo_ir_gradient_processor_new (UfoResources *resources, cl_command_queue cmd_queue);
GType ufo_ir_gradient_processor_get_type (void);

void ufo_ir_gradient_processor_dx_op  (UfoIrGradientProcessor *self, UfoBuffer *input, UfoBuffer *output);
void ufo_ir_gradient_processor_dxt_op (UfoIrGradientProcessor *self, UfoBuffer *input, UfoBuffer *output);
void ufo_ir_gradient_processor_dy_op  (UfoIrGradientProcessor *self, UfoBuffer *input, UfoBuffer *output);
void ufo_ir_gradient_processor_dyt_op (UfoIrGradientProcessor *self, UfoBuffer *input, UfoBuffer *output);
G_END_DECLS

#endif
