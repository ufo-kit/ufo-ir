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

#include "ufo-method-iface.h"

typedef UfoMethodIface UfoMethodInterface;

G_DEFINE_INTERFACE (UfoMethod, ufo_method, G_TYPE_OBJECT)

gboolean
ufo_method_process (UfoMethod *method,
                    UfoBuffer *input,
                    UfoBuffer *output)
{
    g_return_val_if_fail(UFO_IS_METHOD (method) &&
                         UFO_IS_BUFFER (input) &&
                         UFO_IS_BUFFER (output),
                         FALSE);
    return UFO_METHOD_GET_IFACE (method)->process (method, input, output);
}

static gboolean
ufo_method_process_real (UfoMethod *method,
                         UfoBuffer *input,
                         UfoBuffer *output)
{
    g_warning ("%s: `process' not implemented", G_OBJECT_TYPE_NAME (method));
    return FALSE;
}

static void
ufo_method_default_init (UfoMethodInterface *iface)
{
    iface->process = ufo_method_process_real;
}

gpointer
ufo_method_from_json (JsonObject       *object,
                      UfoPluginManager *manager)
{
    /*
    gchar *plugin_name = json_object_get_string_member (object, "plugin");
    gpointer plugin = ufo_plugin_manager_get_plugin (manager,
                                                     METHOD_FUNC_NAME, // depends on method categeory
                                                     plugin_name,
                                                     error);
    gpointer plugin = NULL;
    if (g_strcmp0 (plugin_name, "sart") == 0)
        plugin = ufo_ir_sart_new ();
    else
        plugin = ufo_ir_asdpocs_new ();
    /////////////////////////////////////////////////////

    GList *members = json_object_get_members (object);
    GList *member = NULL;
    gchar *name = NULL;

    for (member = g_list_first (members);
         member != NULL;
         member = g_list_next (member))
    {
        name = member->data;
        if (g_strcmp0 (name, "plugin") == 0) continue;

        JsonNode *node = json_object_get_member(object, name);
        if (JSON_NODE_HOLDS_VALUE (node)) {
            GValue val = {0,};
            json_node_get_value (node, &val);
            g_object_set_property (G_OBJECT(plugin), name, &val);
            g_value_unset (&val);
        }
        else if (JSON_NODE_HOLDS_OBJECT (node)) {

            gpointer inner_obj = ufo_method_from_json (json_node_get_object (node),
                                                       manager);
            g_object_set (plugin, name, inner_obj, NULL);
        }
        else {
            g_warning ("`%s' is neither a primitive value nor an object!", name);
        }
    }
    return plugin;
    #endif*/
    return NULL;
}