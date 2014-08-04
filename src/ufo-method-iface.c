#include <ufo-method-iface.h>
#include <ufo-ir-sart.h>

typedef UfoMethodIface UfoMethodInterface;

G_DEFINE_INTERFACE (UfoMethod, ufo_method, G_TYPE_OBJECT)

GQuark
ufo_method_error_quark ()
{
    return g_quark_from_static_string ("ufo-method-error-quark");
}

gboolean
ufo_method_process (UfoMethod *method,
                    UfoBuffer *input,
                    UfoBuffer *output)
{
    g_return_if_fail(UFO_IS_BUFFER (input) &&
                     UFO_IS_BUFFER (output));
    return UFO_METHOD_GET_IFACE (method)->process (method, input, output);
}

static gboolean
ufo_method_process_real (UfoMethod *method,
                         UfoBuffer *input,
                         UfoBuffer *output)
{
    warn_unimplemented (method, "process");
    return FALSE;
}

static void
ufo_method_default_init (UfoMethodInterface *iface)
{
    iface->process = ufo_method_process_real;
}


gpointer
ufo_method_from_json (JsonObject       *object,
                      UfoPluginManager *manager,
                      GError           **error)
{
    gchar *plugin_name = json_object_get_string_member (object, "plugin");
    /*gpointer plugin = ufo_plugin_manager_get_plugin (manager,
                                            METHOD_FUNC_NAME, // depends on method categeory
                                            plugin_name,
                                            error);*/
    gpointer plugin = ufo_ir_sart_new ();

    GList *members = json_object_get_members (object);
    GList *member = NULL;
    gchar *name = NULL;

    for (member = g_list_first (members);
         member != NULL && !error;
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
                                                       manager,
                                                       error);
            g_object_set (plugin, name, inner_obj, NULL);
        }
        else {
            g_warning ("`%s' is neither a primitive value nor an object!", name);
        }
    }

    return plugin;
}