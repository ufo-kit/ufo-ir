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

#ifndef __UFO_CL_PROJECTOR_H
#define __UFO_CL_PROJECTOR_H

#include <glib.h>
#include <ufo/ufo.h>
#include <ufo/ir/ufo-projector.h>

G_BEGIN_DECLS

#define UFO_TYPE_CL_PROJECTOR            (ufo_cl_projector_get_type())
#define UFO_CL_PROJECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_CL_PROJECTOR, UfoClProjector))
#define UFO_IS_CL_PROJECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_CL_PROJECTOR))
#define UFO_CL_PROJECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_CL_PROJECTOR, UfoClProjectorClass))
#define UFO_IS_CL_PROJECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_CL_PROJECTOR))
#define UFO_CL_PROJECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_CL_PROJECTOR, UfoClProjectorClass))

GQuark ufo_cl_projector_error_quark (void);
#define UFO_CL_PROJECTOR_ERROR          ufo_cl_projector_error_quark()

typedef struct _UfoClProjector         UfoClProjector;
typedef struct _UfoClProjectorClass    UfoClProjectorClass;
typedef struct _UfoClProjectorPrivate  UfoClProjectorPrivate;

typedef enum {
    UFO_CL_PROJECTOR_ERROR_SETUP
} UfoClProjectorError;

struct _UfoClProjector {
    UfoProjector parent_instance;
    UfoClProjectorPrivate *priv;
};

struct _UfoClProjectorClass {
    UfoProjectorClass parent_class;
};

UfoProjector *
ufo_cl_projector_new (void);

GType ufo_cl_projector_get_type (void);
G_END_DECLS
#endif
