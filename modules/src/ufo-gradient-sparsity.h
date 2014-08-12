#ifndef __UFO_GRADIENT_SPARSITY_H
#define __UFO_GRADIENT_SPARSITY_H

#include <glib-object.h>
#include <ufo-processor.h>
#include <ufo-sparsity-iface.h>

G_BEGIN_DECLS

#define UFO_TYPE_GRADIENT_SPARSITY              (ufo_gradient_sparsity_get_type())
#define UFO_GRADIENT_SPARSITY(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_GRADIENT_SPARSITY, UfoGradientSparsity))
#define UFO_IS_GRADIENT_SPARSITY(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_GRADIENT_SPARSITY))
#define UFO_GRADIENT_SPARSITY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_GRADIENT_SPARSITY, UfoGradientSparsityClass))
#define UFO_IS_GRADIENT_SPARSITY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_GRADIENT_SPARSITY)
#define UFO_GRADIENT_SPARSITY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_GRADIENT_SPARSITY, UfoGradientSparsityClass))

typedef struct _UfoGradientSparsity         UfoGradientSparsity;
typedef struct _UfoGradientSparsityClass    UfoGradientSparsityClass;
typedef struct _UfoGradientSparsityPrivate  UfoGradientSparsityPrivate;

struct _UfoGradientSparsity {
    UfoProcessor parent_instance;
    UfoGradientSparsityPrivate *priv;
};

struct _UfoGradientSparsityClass {
    UfoProcessorClass parent_class;
};

UfoSparsity*
ufo_gradient_sparsity_new ();

GType ufo_gradient_sparsity_get_type (void);

G_END_DECLS
#endif