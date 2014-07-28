#ifndef __UFO_PROCESSOR_H
#define __UFO_PROCESSOR_H

#include <glib-object.h>
#include <ufo/ufo.h>
#include <ufo-common-routines.h>

G_BEGIN_DECLS

#define UFO_TYPE_PROCESSOR              (ufo_processor_get_type())
#define UFO_PROCESSOR(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_PROCESSOR, UfoProcessor))
#define UFO_IS_PROCESSOR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_PROCESSOR))
#define UFO_PROCESSOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_PROCESSOR, UfoProcessorClass))
#define UFO_IS_PROCESSOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_PROCESSOR)
#define UFO_PROCESSOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_PROCESSOR, UfoProcessorClass))

GQuark ufo_processor_error_quark (void);
#define UFO_PROCESSOR_ERROR           ufo_processor_error_quark()

typedef struct _UfoProcessor         UfoProcessor;
typedef struct _UfoProcessorClass    UfoProcessorClass;
typedef struct _UfoProcessorPrivate  UfoProcessorPrivate;

typedef enum {
    UFO_PROCESSOR_ERROR_PROCESSING
} UfoProcessorError;

struct _UfoProcessor {
    GObject parent_instance;
    UfoProcessorPrivate *priv;
};

struct _UfoProcessorClass {
    GObjectClass parent_class;

    void (*setup) (UfoProcessor *processor,
                   UfoResources *resources,
                   GError       **error);
};

UfoProcessor*
ufo_processor_new ();

void
ufo_processor_setup (UfoProcessor *processor,
                     UfoResources *resources,
                     GError       **error);

void
ufo_processor_process (UfoProcessor *processor,
                       UfoBuffer    *input,
                       UfoBuffer    *output);

GType ufo_processor_get_type (void);
G_END_DECLS
#endif
