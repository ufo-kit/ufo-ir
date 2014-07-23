#ifndef __UFO_METHOD_H
#define __UFO_METHOD_H

#include <glib-object.h>
#include <ufo/ufo.h>

G_BEGIN_DECLS

#define UFO_TYPE_METHOD              (ufo_method_get_type())
#define UFO_METHOD(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_METHOD, UfoMethod))
#define UFO_IS_METHOD(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_METHOD))
#define UFO_METHOD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_METHOD, UfoMethodClass))
#define UFO_IS_METHOD_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_METHOD)
#define UFO_METHOD_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_METHOD, UfoMethodClass))

GQuark ufo_method_error_quark (void);
#define UFO_METHOD_ERROR           ufo_method_error_quark()

typedef struct _UfoMethod         UfoMethod;
typedef struct _UfoMethodClass    UfoMethodClass;
typedef struct _UfoMethodPrivate  UfoMethodPrivate;

typedef enum {
    UFO_METHOD_ERROR_PROCESSING
} UfoMethodError;

struct _UfoMethod {
    GObject parent_instance;
    UfoMethodPrivate *priv;
};

struct _UfoMethodClass {
    GObjectClass parent_class;

    void (*setup) (UfoMethod  *method,
                   UfoResources *resources,
                   GError       **error);

    void (*process) (UfoMethod  *method,
                     UfoBuffer *input,
                     UfoBuffer *output);
};

UfoMethod*
ufo_method_new ();

void
ufo_method_setup (UfoMethod  *method,
                  UfoResources *resources,
                  GError       **error);

void
ufo_method_process (UfoMethod  *method,
                    UfoBuffer *input,
                    UfoBuffer *output);

GType ufo_method_get_type (void);
G_END_DECLS
#endif
