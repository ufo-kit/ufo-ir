/*
* Copyright (C) 2011-2013 Karlsruhe Institute of Technology
*
* This file is part of Ufo.
*
* This library is free software: you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation, either
* version 3 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UFO_PRIOR_KNOWLEDGE_H
#define UFO_PRIOR_KNOWLEDGE_H

#include <glib.h>
#include <glib-object.h>
#include <ufo/ufo.h>

typedef GHashTable UfoPriorKnowledge;

UfoPriorKnowledge *
ufo_prior_knowledge_new (void);

void
ufo_prior_knowledge_set_boolean (UfoPriorKnowledge *prior,
                                 gchar             *key,
                                 gboolean          value);

gboolean
ufo_prior_knowledge_boolean (UfoPriorKnowledge *prior,
                             gchar             *key);

void
ufo_prior_knowledge_set_pointer (UfoPriorKnowledge *prior,
                                 gchar             *key,
                                 gpointer          obj);
gpointer
ufo_prior_knowledge_pointer (UfoPriorKnowledge *prior,
                             gchar             *key);

gpointer
ufo_prior_knowledge_from_json (JsonObject       *object,
                               UfoPluginManager *manager);

#endif