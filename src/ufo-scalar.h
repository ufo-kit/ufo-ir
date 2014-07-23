#ifndef __UFO_IR_SCALAR_H
#define __UFO_IR_SCALAR_H

#include <glib-object.h>

typedef gfloat UfoScalar;
#define UFO_IR_TYPE_SCALAR G_TYPE_FLOAT
#define UFO_IR_MIN_SCALAR  (-G_MAXFLOAT)
#define UFO_IR_MAX_SCALAR  (G_MAXFLOAT)

UfoScalar
g_value_get_scalar (const GValue *value);

void
g_value_set_scalar (GValue *value, UfoScalar val);

GParamSpec *
g_param_spec_scalar (const gchar *name,
                     const gchar *nick,
                     const gchar *blurb,
                     UfoScalar minimum,
                     UfoScalar maximum,
                     UfoScalar default_value,
                     GParamFlags flags);

#endif