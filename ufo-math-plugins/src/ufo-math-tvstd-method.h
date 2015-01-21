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

#ifndef __UFO_MATH_TVSTD_METHOD_H
#define __UFO_MATH_TVSTD_METHOD_H

#include <glib-object.h>
#include <ufo/ufo.h>

G_BEGIN_DECLS

#define UFO_MATH_TYPE_TVSTD_METHOD              (ufo_math_tvstd_method_get_type())
#define UFO_MATH_TVSTD_METHOD(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_MATH_TYPE_TVSTD_METHOD, UfoMathTvStdMethod))
#define UFO_MATH_IS_TVSTD_METHOD(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_MATH_TYPE_TVSTD_METHOD))
#define UFO_MATH_TVSTD_METHOD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UFO_MATH_TYPE_TVSTD_METHOD, UfoMathTvStdMethodClass))
#define UFO_MATH_IS_TVSTD_METHOD_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_MATH_TYPE_TVSTD_METHOD)
#define UFO_MATH_TVSTD_METHOD_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_MATH_TYPE_TVSTD_METHOD, UfoMathTvStdMethodClass))

GQuark ufo_math_tvstd_method_error_quark (void);
#define UFO_MATH_TVSTD_METHOD_ERROR ufo_math_tvstd_method_error_quark()

typedef enum {
    UFO_MATH_TVSTD_METHOD_ERROR_SETUP
} UfoMathTvStdMethodError;

typedef struct _UfoMathTvStdMethod         UfoMathTvStdMethod;
typedef struct _UfoMathTvStdMethodClass    UfoMathTvStdMethodClass;
typedef struct _UfoMathTvStdMethodPrivate  UfoMathTvStdMethodPrivate;

struct _UfoMathTvStdMethod {
    UfoProcessor parent_instance;
    UfoMathTvStdMethodPrivate *priv;
};

struct _UfoMathTvStdMethodClass {
    UfoProcessorClass parent_class;
};

UfoMethod    *ufo_math_tvstd_method_new       (void);
GType         ufo_math_tvstd_method_get_type  (void);
G_END_DECLS
#endif
