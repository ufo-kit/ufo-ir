#ifndef UFO_SPARSITY_IFACE_H
#define UFO_SPARSITY_IFACE_H

#include <ufo-processor.h>

G_BEGIN_DECLS

#define UFO_TYPE_SPARSITY (ufo_sparsity_get_type())
#define UFO_SPARSITY(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_SPARSITY, UfoSparsity))
#define UFO_SPARSITY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_SPARSITY, UfoSparsityIface))
#define UFO_IS_SPARSITY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_SPARSITY))
#define UFO_IS_SPARSITY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_SPARSITY))
#define UFO_SPARSITY_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE((inst), UFO_TYPE_SPARSITY, UfoSparsityIface))

#define UFO_SPARSITY_ERROR ufo_sparsity_error_quark()

typedef struct _UfoSparsity UfoSparsity;
typedef struct _UfoSparsityIface UfoSparsityIface;

struct _UfoSparsityIface {
    /*< private >*/
    GTypeInterface parent_iface;

    gboolean (*minimize) (UfoSparsity *sparsity,
                          UfoBuffer *input,
                          UfoBuffer *output);
};

gboolean
ufo_sparsity_minimize (UfoSparsity *sparsity,
                       UfoBuffer   *input,
                       UfoBuffer   *output);

GQuark ufo_sparsity_error_quark (void);
GType ufo_sparsity_get_type (void);

G_END_DECLS
#endif