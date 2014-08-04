#ifndef __UFO_COMMON_ROUTINES_H
#define __UFO_COMMON_ROUTINES_H

#include <glib.h>
#include <glib-object.h>
#include <ufo/ufo.h>

void warn_unimplemented (gpointer     object,
                         const gchar *func);

void error_unimplemented (gpointer     object,
                          const gchar *func);

#endif