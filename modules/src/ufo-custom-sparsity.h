#ifndef __UFO_CUSTOM_SPARSITY_H
#define __UFO_CUSTOM_SPARSITY_H

#include <glib-object.h>
#include <ufo/ir/ufo-processor.h>
#include <ufo/ir/ufo-sparsity-iface.h>
#include <ufo/ir/ufo-method-iface.h>
#include <ufo/ir/ufo-transform-iface.h>

G_BEGIN_DECLS

#define UFO_TYPE_CUSTOM_SPARSITY              (ufo_custom_sparsity_get_type())
#define UFO_CUSTOM_SPARSITY(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_CUSTOM_SPARSITY, UfoCustomSparsity))
#define UFO_IS_CUSTOM_SPARSITY(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_CUSTOM_SPARSITY))
#define UFO_CUSTOM_SPARSITY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_CUSTOM_SPARSITY, UfoCustomSparsityClass))
#define UFO_IS_CUSTOM_SPARSITY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_CUSTOM_SPARSITY)
#define UFO_CUSTOM_SPARSITY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_CUSTOM_SPARSITY, UfoCustomSparsityClass))

GQuark ufo_custom_sparsity_error_quark (void);
#define UFO_CUSTOM_SPARSITY_ERROR           ufo_custom_sparsity_error_quark()

typedef struct _UfoCustomSparsity         UfoCustomSparsity;
typedef struct _UfoCustomSparsityClass    UfoCustomSparsityClass;
typedef struct _UfoCustomSparsityPrivate  UfoCustomSparsityPrivate;


struct _UfoCustomSparsity {
    UfoProcessor parent_instance;
    UfoCustomSparsityPrivate *priv;
};

struct _UfoCustomSparsityClass {
    UfoProcessorClass parent_class;
};

UfoSparsity*
ufo_custom_sparsity_new (void);

GType ufo_custom_sparsity_get_type (void);

G_END_DECLS
#endif
