#ifndef UFO_METHODS_COMMON_ROUTINES_H
#define UFO_METHODS_COMMON_ROUTINES_H

#include <glib-object.h>
#include <ufo/ufo.h>

gpointer    ufo_object_from_json (JsonObject       *object,
                                  UfoPluginManager *manager);

#endif