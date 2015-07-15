#include "ufo-ir-debug.h"

void
ufo_ir_debug_write_image(UfoBuffer *buffer,
                         const char *file_name) {
    //return;
    UfoPluginManager *manager = ufo_plugin_manager_new();
    UfoTask *writer = UFO_TASK(ufo_plugin_manager_get_task (manager, "write", NULL));
    //ufo_task_node_set_proc_node(UFO_TASK_NODE(writer), proc_node);
    g_object_set (G_OBJECT (writer), "filename", file_name, NULL);
    GError *error = NULL;
    ufo_task_setup(writer, NULL, &error);
    if (error)
    {
        g_printerr("\nError: SBTV ifft setup: %s\n", error->message);
        return;
    }

    UfoBuffer *inputs[1]; // temp arrey for invocing get_requisition method
    inputs[0] = buffer;
    ufo_task_process(writer, inputs, NULL, NULL);

    g_object_unref(manager);
    g_object_unref(writer);
}
