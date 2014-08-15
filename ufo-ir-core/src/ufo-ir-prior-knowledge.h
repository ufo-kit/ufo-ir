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

#ifndef UFO_IR_PRIOR_KNOWLEDGE_H
#define UFO_IR_PRIOR_KNOWLEDGE_H

#include <glib.h>
#include <glib-object.h>
#include <ufo/ufo.h>

typedef GHashTable UfoIrPriorKnowledge;

UfoIrPriorKnowledge * ufo_ir_prior_knowledge_new (void);

void     ufo_ir_prior_knowledge_set_boolean (UfoIrPriorKnowledge *prior,
                                             const gchar         *key,
                                             gboolean            value);
gboolean ufo_ir_prior_knowledge_boolean     (UfoIrPriorKnowledge *prior,
                                             const gchar         *key);
void     ufo_ir_prior_knowledge_set_pointer (UfoIrPriorKnowledge *prior,
                                             const gchar         *key,
                                             gpointer            obj);
gpointer ufo_ir_prior_knowledge_pointer     (UfoIrPriorKnowledge *prior,
                                             const gchar         *key);
gpointer ufo_ir_prior_knowledge_from_json   (JsonObject       *object,
                                             UfoPluginManager *manager);
#endif