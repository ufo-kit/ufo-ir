#include <ufo-scalar.h>

UfoScalar
g_value_get_scalar (const GValue *value) {
    return g_value_get_float (value);
}

void
g_value_set_scalar (GValue *value, UfoScalar val)
{
    g_value_set_float (value, val);
}

GParamSpec *
g_param_spec_scalar (const gchar *name,
                     const gchar *nick,
                     const gchar *blurb,
                     UfoScalar minimum,
                     UfoScalar maximum,
                     UfoScalar default_value,
                     GParamFlags flags)
{
    return g_param_spec_float (name, nick, blurb, minimum, maximum, default_value, flags);
}
