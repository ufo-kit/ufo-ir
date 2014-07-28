#ifndef __UFO_IR_METHOD_H
#define __UFO_IR_METHOD_H

#include <glib-object.h>
#include <ufo-processor.h>
#include <ufo-method-iface.h>
#include <ufo-projector.h>
#include <ufo-geometry.h>

G_BEGIN_DECLS

#define UFO_TYPE_IR_METHOD              (ufo_ir_method_get_type())
#define UFO_IR_METHOD(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_IR_METHOD, UfoIrMethod))
#define UFO_IS_IR_METHOD(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_IR_METHOD))
#define UFO_IR_METHOD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_IR_METHOD, UfoIrMethodClass))
#define UFO_IS_IR_METHOD_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_IR_METHOD)
#define UFO_IR_METHOD_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_IR_METHOD, UfoIrMethodClass))

GQuark ufo_ir_method_error_quark (void);
#define UFO_IR_METHOD_ERROR           ufo_ir_method_error_quark()

typedef struct _UfoIrMethod         UfoIrMethod;
typedef struct _UfoIrMethodClass    UfoIrMethodClass;
typedef struct _UfoIrMethodPrivate  UfoIrMethodPrivate;


struct _UfoIrMethod {
    UfoProcessor parent_instance;
    UfoIrMethodPrivate *priv;
};

struct _UfoIrMethodClass {
    UfoProcessorClass parent_class;

    void (*set_prior_knowledge) (UfoIrMethod *method,
                                 GHashTable  *prior);
};

UfoMethod*
ufo_ir_method_new ();

void
ufo_ir_method_set_prior_knowledge (UfoIrMethod *method,
                                   GHashTable  *prior);

GType ufo_ir_method_get_type (void);

G_END_DECLS
#endif
