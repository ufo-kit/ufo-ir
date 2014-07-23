#ifndef __UFO_METADATA_H
#define __UFO_METADATA_H

#include <glib.h>
#include <ufo-scalar.h>
typedef GHashTable UfoMetaData;

UfoMetaData *
ufo_metadata_new ();

gchar *
ufo_metadata_string (UfoMetaData *md,
                     const gchar *name);

UfoScalar
ufo_metadata_scalar (UfoMetaData *md,
                     const gchar *name);

gpointer
ufo_metadata_pointer (UfoMetaData *md,
                     const gchar *name);

void
ufo_metadata_set_string (UfoMetaData *md,
                         const gchar *name,
                         gchar       *value);

void
ufo_metadata_set_scalar (UfoMetaData *md,
                         const gchar *name,
                         UfoScalar   value);

void
ufo_metadata_set_pointer (UfoMetaData *md,
                          const gchar *name,
                          gpointer    *value);

#endif
