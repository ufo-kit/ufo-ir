#include <ufo-metadata.h>

UfoMetaData *
ufo_metadata_new ()
{
    return g_hash_table_new (g_str_hash, g_str_equal);
}

gchar *
ufo_metadata_string (UfoMetaData *md,
                     const gchar *name)
{
    return g_hash_table_lookup (md, name);
}

UfoScalar
ufo_metadata_scalar (UfoMetaData *md,
                     const gchar *name)
{
    GValue *scalar_value = g_hash_table_lookup (md, name);
    return g_value_get_scalar(scalar_value);
}

gpointer
ufo_metadata_pointer (UfoMetaData *md,
                     const gchar *name)
{
    return g_hash_table_lookup (md, name);
}

void
ufo_metadata_set_string (UfoMetaData *md,
                         const gchar *name,
                         gchar       *value)
{
    g_hash_table_replace (md, name, value);
}

void
ufo_metadata_set_scalar (UfoMetaData *md,
                         const gchar *name,
                         UfoScalar    value)
{
    GValue *scalar_value = g_hash_table_lookup (md, name);
    if (!G_IS_VALUE(scalar_value)) {
        scalar_value = g_malloc0(sizeof(GValue));
        g_value_init (scalar_value, UFO_IR_TYPE_SCALAR);
        g_hash_table_replace (md, name, scalar_value);
    }

    g_value_set_scalar (scalar_value, value);
}

void
ufo_metadata_set_pointer (UfoMetaData *md,
                          const gchar *name,
                          gpointer    *value)
{
    g_hash_table_replace (md, name, value);
}