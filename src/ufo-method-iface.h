#ifndef UFO_METHOD_IFACE_H
#define UFO_METHOD_IFACE_H

#include <ufo-processor.h>

G_BEGIN_DECLS

#define UFO_TYPE_METHOD (ufo_method_get_type())
#define UFO_METHOD(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_METHOD, UfoMethod))
#define UFO_METHOD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_METHOD, UfoMethodIface))
#define UFO_IS_METHOD(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_METHOD))
#define UFO_IS_METHOD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_METHOD))
#define UFO_METHOD_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE((inst), UFO_TYPE_METHOD, UfoMethodIface))

#define UFO_METHOD_ERROR ufo_method_error_quark()

typedef struct _UfoMethod UfoMethod;
typedef struct _UfoMethodIface UfoMethodIface;

struct _UfoMethodIface {
    /*< private >*/
    GTypeInterface parent_iface;

    gboolean (*process) (UfoMethod *method,
                         UfoBuffer *input,
                         UfoBuffer *output);
};

gboolean
ufo_method_process (UfoMethod *method,
                    UfoBuffer *input,
                    UfoBuffer *output);

GQuark ufo_method_error_quark (void);
GType ufo_method_get_type (void);

G_END_DECLS
#endif