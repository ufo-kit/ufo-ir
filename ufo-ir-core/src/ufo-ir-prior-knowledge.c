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

#include "ufo-ir-prior-knowledge.h"

UfoIrPriorKnowledge *
ufo_ir_prior_knowledge_new (void)
{
    return g_hash_table_new (g_str_hash, g_str_equal);
}

void
ufo_ir_prior_knowledge_set_boolean (UfoIrPriorKnowledge *prior,
                                    const gchar         *key,
                                    gboolean            value)
{
    GValue *gval = g_hash_table_lookup (prior, key);
    if (!gval) {
        gval = g_malloc0(sizeof(GValue));
        g_value_init (gval, G_TYPE_BOOLEAN);
        g_hash_table_insert (prior, g_strdup(key), gval);
    }
    g_value_set_boolean (gval, value);
}

gboolean
ufo_ir_prior_knowledge_boolean (UfoIrPriorKnowledge *prior,
                                const gchar         *key)
{
    GValue *gval = g_hash_table_lookup (prior, key);
    return gval ? g_value_get_boolean (gval) : FALSE;
}

void
ufo_ir_prior_knowledge_set_pointer (UfoIrPriorKnowledge *prior,
                                    const gchar         *key,
                                    gpointer            obj)
{
    g_hash_table_insert (prior, g_strdup(key), obj);
}

gpointer
ufo_ir_prior_knowledge_pointer (UfoIrPriorKnowledge *prior,
                                const gchar         *key)
{
    return g_hash_table_lookup (prior, key);
}

gpointer
ufo_ir_prior_knowledge_from_json (JsonObject       *object,
                                  UfoPluginManager *manager)
{
    UfoIrPriorKnowledge   *prior = ufo_ir_prior_knowledge_new ();
    //UfoSparsity *sparsity = ufo_gradient_sparsity_new ();
    //ufo_ir_prior_knowledge_set_boolean (prior, "phase-contrast", TRUE);
    //ufo_ir_prior_knowledge_set_pointer (prior, "image-sparsity", sparsity);

    GList *members = json_object_get_members (object);
    GList *member = NULL;
    gchar *name = NULL;

    for (member = g_list_first (members);
         member != NULL;
         member = g_list_next (member))
    {
        name = member->data;
        g_print ("\n Prior: %s", name);
        /*
        JsonNode *node = json_object_get_member(object, name);
        if (JSON_NODE_HOLDS_VALUE (node)) {
            GValue val = {0,};
            json_node_get_value (node, &val);
            g_object_set_property (G_OBJECT(plugin), name, &val);
            g_value_unset (&val);
        }*/
    }

    GError *tmp_error = NULL;
    gpointer plugin = ufo_plugin_manager_get_plugin (manager,
                                                     "ufo_ir_gradient_sparsity_new",
                                                     "libufoir_gradient_sparsity.so",
                                                     &tmp_error);
    if (tmp_error != NULL) {
      g_warning ("%s", tmp_error->message);
      return NULL;
    }
    ufo_ir_prior_knowledge_set_pointer (prior, "image-sparsity", plugin);

    return prior;
}