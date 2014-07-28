#ifndef UFO_TRANSFORM_IFACE_H
#define UFO_TRANSFORM_IFACE_H

#include <ufo-processor.h>

G_BEGIN_DECLS

#define UFO_TYPE_TRANSFORM (ufo_transform_get_type())
#define UFO_TRANSFORM(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_TRANSFORM, UfoTransform))
#define UFO_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_TRANSFORM, UfoTransformIface))
#define UFO_IS_TRANSFORM(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_TRANSFORM))
#define UFO_IS_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_TRANSFORM))
#define UFO_TRANSFORM_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE((inst), UFO_TYPE_TRANSFORM, UfoTransformIface))

#define UFO_TRANSFORM_ERROR ufo_transform_error_quark()

typedef struct _UfoTransform UfoTransform;
typedef struct _UfoTransformIface UfoTransformIface;

struct _UfoTransformIface {
    /*< private >*/
    GTypeInterface parent_iface;

    gboolean (*direct) (UfoTransform *transform,
                        UfoBuffer *input,
                        UfoBuffer *output);

    gboolean (*inverse) (UfoTransform *transform,
                         UfoBuffer *input,
                         UfoBuffer *output);
};

gboolean
ufo_transform_direct (UfoTransform *transform,
                      UfoBuffer *input,
                      UfoBuffer *output);
gboolean
ufo_transform_inverse (UfoTransform *transform,
                       UfoBuffer *input,
                       UfoBuffer *output);

GQuark ufo_transform_error_quark (void);
GType ufo_transform_get_type (void);

G_END_DECLS
#endif