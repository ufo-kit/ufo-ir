#ifndef __UFO_CL_PROJECTOR_H
#define __UFO_CL_PROJECTOR_H

#include <glib.h>
#include <ufo/ufo.h>
#include "ufo-projector.h"

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
ufo_cl_projector_new ();

GType ufo_cl_projector_get_type (void);
G_END_DECLS
#endif
