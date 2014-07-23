#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <ufo-ir-sart.h>
#include <ufo/ufo.h>

G_DEFINE_TYPE (UfoIrSART, ufo_ir_sart, UFO_TYPE_IR_METHOD)
#define UFO_IR_SART_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_IR_SART, UfoIrSARTPrivate))

GQuark
ufo_ir_sart_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-sart-error-quark");
}

struct _UfoIrSARTPrivate {
    UfoBuffer *singular_volume;
};


UfoIrMethod *
ufo_ir_sart_new ()
{
    UfoIrMethod *method =
        (UfoIrMethod *) g_object_new (UFO_TYPE_IR_SART,
                                      NULL);
    return method;
}

static void
ufo_ir_sart_init (UfoIrSART *self)
{
    UfoIrSARTPrivate *priv = NULL;
    self->priv = priv = UFO_IR_SART_GET_PRIVATE (self);
    priv->singular_volume = NULL;
}

static void
_ufo_ir_sart_setup (UfoIrMethod  *method,
                    UfoResources *resources,
                    GError       **error)
{
    UFO_METHOD_CLASS (ufo_ir_sart_parent_class)->setup (method, resources, error);
    g_print ("\n_ufo_ir_sart_setup");
}

static void
ufo_ir_sart_class_init (UfoIrSARTClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (gobject_class, sizeof(UfoIrSARTPrivate));

    UFO_METHOD_CLASS (klass)->setup = _ufo_ir_sart_setup;

//    parent_class->process = _ufo_ir_sart_process;
}
