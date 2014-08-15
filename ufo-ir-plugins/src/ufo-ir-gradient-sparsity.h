#ifndef __UFO_IR_GRADIENT_SPARSITY_H
#define __UFO_IR_GRADIENT_SPARSITY_H

#include <glib-object.h>
#include <ufo/methods/ufo-processor.h>
#include <ufo/ir/ufo-ir-sparsity-iface.h>

G_BEGIN_DECLS

#define UFO_IR_TYPE_GRADIENT_SPARSITY              (ufo_ir_gradient_sparsity_get_type())
#define UFO_IR_GRADIENT_SPARSITY(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_GRADIENT_SPARSITY, UfoIrGradientSparsity))
#define UFO_IR_IS_GRADIENT_SPARSITY(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_GRADIENT_SPARSITY))
#define UFO_IR_GRADIENT_SPARSITY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_GRADIENT_SPARSITY, UfoIrGradientSparsityClass))
#define UFO_IR_IS_GRADIENT_SPARSITY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_GRADIENT_SPARSITY)
#define UFO_IR_GRADIENT_SPARSITY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_GRADIENT_SPARSITY, UfoIrGradientSparsityClass))

typedef struct _UfoIrGradientSparsity         UfoIrGradientSparsity;
typedef struct _UfoIrGradientSparsityClass    UfoIrGradientSparsityClass;
typedef struct _UfoIrGradientSparsityPrivate  UfoIrGradientSparsityPrivate;

struct _UfoIrGradientSparsity {
    UfoProcessor parent_instance;
    UfoIrGradientSparsityPrivate *priv;
};

struct _UfoIrGradientSparsityClass {
    UfoProcessorClass parent_class;
};

UfoIrSparsity  *ufo_ir_gradient_sparsity_new        (void);
GType           ufo_ir_gradient_sparsity_get_type   (void);

G_END_DECLS
#endif
