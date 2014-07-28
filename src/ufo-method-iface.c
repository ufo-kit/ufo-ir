#include <ufo-method-iface.h>

typedef UfoMethodIface UfoMethodInterface;

G_DEFINE_INTERFACE (UfoMethod, ufo_method, G_TYPE_OBJECT)

GQuark
ufo_method_error_quark ()
{
    return g_quark_from_static_string ("ufo-method-error-quark");
}

gboolean
ufo_method_process (UfoMethod *method,
                    UfoBuffer *input,
                    UfoBuffer *output)
{
    g_return_if_fail(UFO_IS_BUFFER (input) &&
                     UFO_IS_BUFFER (output));
    return UFO_METHOD_GET_IFACE (method)->process (method, input, output);
}

static gboolean
ufo_method_process_real (UfoMethod *method,
                         UfoBuffer *input,
                         UfoBuffer *output)
{
    warn_unimplemented (method, "process");
    return FALSE;
}

static void
ufo_method_default_init (UfoMethodInterface *iface)
{
    iface->process = ufo_method_process_real;
}