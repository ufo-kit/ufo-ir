#include <ufo-transform-iface.h>

typedef UfoTransformIface UfoTransformInterface;

G_DEFINE_INTERFACE (UfoTransform, ufo_transform, G_TYPE_OBJECT)

GQuark
ufo_transform_error_quark ()
{
    return g_quark_from_static_string ("ufo-transform-error-quark");
}

gboolean
ufo_transform_direct (UfoTransform *transform,
                      UfoBuffer *input,
                      UfoBuffer *output)
{
    g_return_if_fail(UFO_IS_BUFFER (input) &&
                     UFO_IS_BUFFER (output));

    return UFO_TRANSFORM_GET_IFACE (transform)->direct (transform, input, output);
}

gboolean
ufo_transform_inverse (UfoTransform *transform,
                       UfoBuffer *input,
                       UfoBuffer *output)
{
    g_return_if_fail(UFO_IS_BUFFER (input) &&
                     UFO_IS_BUFFER (output));

    return UFO_TRANSFORM_GET_IFACE (transform)->inverse (transform, input, output);
}

static gboolean
ufo_transform_direct_real (UfoTransform *transform,
                            UfoBuffer *input,
                            UfoBuffer *output)
{
    warn_unimplemented (transform, "direct");
    return FALSE;
}

static gboolean
ufo_transform_inverse_real (UfoTransform *transform,
                            UfoBuffer *input,
                            UfoBuffer *output)
{
    warn_unimplemented (transform, "inverse");
    return FALSE;
}

static void
ufo_transform_default_init (UfoTransformInterface *iface)
{
    iface->direct = ufo_transform_direct_real;
    iface->inverse = ufo_transform_inverse_real;
}