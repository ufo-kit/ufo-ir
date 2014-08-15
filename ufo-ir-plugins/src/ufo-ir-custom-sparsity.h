#ifndef __UFO_IR_CUSTOM_SPARSITY_H
#define __UFO_IR_CUSTOM_SPARSITY_H

#include <glib-object.h>
#include <ufo/methods/ufo-processor.h>
#include <ufo/methods/ufo-method-iface.h>
#include <ufo/methods/ufo-transform-iface.h>
#include <ufo/ir/ufo-ir-sparsity-iface.h>

G_BEGIN_DECLS

#define UFO_IR_TYPE_CUSTOM_SPARSITY              (ufo_ir_custom_sparsity_get_type())
#define UFO_IR_CUSTOM_SPARSITY(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_IR_TYPE_CUSTOM_SPARSITY, UfoIrCustomSparsity))
#define UFO_IR_IS_CUSTOM_SPARSITY(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_IR_TYPE_CUSTOM_SPARSITY))
#define UFO_IR_CUSTOM_SPARSITY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UFO_IR_TYPE_CUSTOM_SPARSITY, UfoIrCustomSparsityClass))
#define UFO_IR_IS_CUSTOM_SPARSITY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_IR_TYPE_CUSTOM_SPARSITY)
#define UFO_IR_CUSTOM_SPARSITY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_IR_TYPE_CUSTOM_SPARSITY, UfoIrCustomSparsityClass))

GQuark ufo_ir_custom_sparsity_error_quark (void);
#define UFO_IR_CUSTOM_SPARSITY_ERROR           ufo_ir_custom_sparsity_error_quark()

typedef struct _UfoIrCustomSparsity         UfoIrCustomSparsity;
typedef struct _UfoIrCustomSparsityClass    UfoIrCustomSparsityClass;
typedef struct _UfoIrCustomSparsityPrivate  UfoIrCustomSparsityPrivate;


struct _UfoIrCustomSparsity {
    UfoProcessor parent_instance;
    UfoIrCustomSparsityPrivate *priv;
};

struct _UfoIrCustomSparsityClass {
    UfoProcessorClass parent_class;
};

UfoIrSparsity   *ufo_ir_custom_sparsity_new         (void);
GType            ufo_ir_custom_sparsity_get_type    (void);

G_END_DECLS
#endif
