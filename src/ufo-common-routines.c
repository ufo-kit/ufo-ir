#include <ufo-common-routines.h>

void
warn_unimplemented (gpointer     object,
                    const gchar *func)
{
    g_warning ("%s: `%s' not implemented", G_OBJECT_TYPE_NAME (object), func);
}