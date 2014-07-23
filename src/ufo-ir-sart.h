#ifndef __UFO_IR_SART_H
#define __UFO_IR_SART_H

#include <glib-object.h>
#include <ufo-ir-method.h>

G_BEGIN_DECLS

#define UFO_TYPE_IR_SART              (ufo_ir_sart_get_type())
#define UFO_IR_SART(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_IR_SART, UfoIrSART))
#define UFO_IS_IR_SART(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_IR_SART))
#define UFO_IR_SART_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_IR_SART, UfoIrSARTClass))
#define UFO_IS_IR_SART_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_IR_SART)
#define UFO_IR_SART_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_IR_SART, UfoIrSARTClass))

GQuark ufo_ir_sart_error_quark (void);
#define UFO_IR_SART_ERROR           ufo_ir_sart_error_quark()

typedef struct _UfoIrSART         UfoIrSART;
typedef struct _UfoIrSARTClass    UfoIrSARTClass;
typedef struct _UfoIrSARTPrivate  UfoIrSARTPrivate;

struct _UfoIrSART {
    UfoIrMethod parent_instance;
    UfoIrSARTPrivate *priv;
};

struct _UfoIrSARTClass {
    UfoIrMethodClass parent_class;
};

UfoIrMethod*
ufo_ir_sart_new ();

GType ufo_ir_sart_get_type (void);
G_END_DECLS
#endif
