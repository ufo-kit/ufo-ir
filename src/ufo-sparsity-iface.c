#include <ufo-sparsity-iface.h>

typedef UfoSparsityIface UfoSparsityInterface;

G_DEFINE_INTERFACE (UfoSparsity, ufo_sparsity, G_TYPE_OBJECT)

GQuark
ufo_sparsity_error_quark ()
{
    return g_quark_from_static_string ("ufo-sparsity-error-quark");
}

gboolean
ufo_sparsity_minimize (UfoSparsity *sparsity,
                       UfoBuffer *input,
                       UfoBuffer *output)
{
    /*g_return_if_fail(UFO_IS_BUFFER (input) &&
                     UFO_IS_BUFFER (output));*/
    return UFO_SPARSITY_GET_IFACE (sparsity)->minimize (sparsity, input, output);
}

static gboolean
ufo_sparsity_minimize_real (UfoSparsity *sparsity,
                            UfoBuffer *input,
                            UfoBuffer *output)
{
    warn_unimplemented (sparsity, "minimize");
    return FALSE;
}

static void
ufo_sparsity_default_init (UfoSparsityInterface *iface)
{
    iface->minimize = ufo_sparsity_minimize_real;
}