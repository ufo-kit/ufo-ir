#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <ufo-ir-asdpocs.h>
#include <ufo/ufo.h>
#include <math.h>

static void ufo_method_interface_init (UfoMethodIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrASDPOCS, ufo_ir_asdpocs, UFO_TYPE_IR_METHOD,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_METHOD,
                                                ufo_method_interface_init))

#define UFO_IR_ASDPOCS_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_IR_ASDPOCS, UfoIrASDPOCSPrivate))

struct _UfoIrASDPOCSPrivate {
  UfoIrMethod *df_minimizer;
};

enum {
    PROP_0,
    PROP_DF_MINIMIZER,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoIrMethod *
ufo_ir_asdpocs_new ()
{
    return UFO_IR_METHOD (g_object_new (UFO_TYPE_IR_ASDPOCS, NULL));
}

static void
ufo_ir_asdpocs_init (UfoIrASDPOCS *self)
{
    UfoIrASDPOCSPrivate *priv = NULL;
    self->priv = priv = UFO_IR_ASDPOCS_GET_PRIVATE (self);
    priv->df_minimizer = NULL;
}

static void
ufo_ir_asdpocs_set_property (GObject      *object,
                             guint        property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
    UfoIrASDPOCSPrivate *priv = UFO_IR_ASDPOCS_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_DF_MINIMIZER:
            g_clear_object(&priv->df_minimizer);
            priv->df_minimizer = g_object_ref (g_value_get_pointer(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_asdpocs_get_property (GObject    *object,
                             guint      property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
    UfoIrASDPOCSPrivate *priv = UFO_IR_ASDPOCS_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_DF_MINIMIZER:
            g_value_set_pointer (value, priv->df_minimizer);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_asdpocs_dispose (GObject *object)
{
    UfoIrASDPOCSPrivate *priv = UFO_IR_ASDPOCS_GET_PRIVATE (object);
    g_clear_object(&priv->df_minimizer);

    G_OBJECT_CLASS (ufo_ir_asdpocs_parent_class)->dispose (object);
}

static gboolean
ufo_ir_asdpocs_process_real (UfoMethod *method,
                             UfoBuffer *input,
                             UfoBuffer *output)
{
    g_print ("\nufo_ir_asdpocs_process_real\n");
    UfoIrASDPOCSPrivate *priv = UFO_IR_ASDPOCS_GET_PRIVATE (method);
    ufo_method_process (UFO_METHOD (priv->df_minimizer), input, output);
    return TRUE;
}

static void
ufo_method_interface_init (UfoMethodIface *iface)
{
    iface->process = ufo_ir_asdpocs_process_real;
}

static void
ufo_ir_asdpocs_class_init (UfoIrASDPOCSClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = ufo_ir_asdpocs_dispose;
    gobject_class->set_property = ufo_ir_asdpocs_set_property;
    gobject_class->get_property = ufo_ir_asdpocs_get_property;

    properties[PROP_DF_MINIMIZER] =
        g_param_spec_pointer("df-minimizer",
                             "Pointer to the instance of UfoIrMethod.",
                             "Pointer to the instance of UfoIrMethod",
                             G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof(UfoIrASDPOCSPrivate));
}
