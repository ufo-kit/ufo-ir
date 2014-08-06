#include <stdio.h>
#include <glib.h>
#include <ufo/ufo.h>
#include <math.h>

#include <ufo-ir-method.h>
#include <ufo-ir-sart.h>

#include <ufo-sparsity-iface.h>
#include <ufo-custom-sparsity.h>
//#include <ufo-geom.h>

void
test_task ();
int main(int n_args, char *argv[])
{
    /*UfoRequisition req;
    req.n_dims = 2;
    req.dims[0] = 10;
    req.dims[1] = 20;

    UfoBuffer *buffer1 = ufo_buffer_new (&req, NULL);
    UfoBuffer *buffer2 = ufo_buffer_new (&req, NULL);
    UfoIrMethod *m = ufo_ir_sart_new ();
    ufo_method_process (UFO_METHOD(m), buffer1, buffer2);

    UfoSparsity *cs = ufo_custom_sparsity_new ();
    ufo_sparsity_minimize (cs, buffer1, buffer2);
*/
    //g_object_unref (NULL);
    test_task ();
/*
    UfoGeom *geom = ufo_geom_new();
    gchar *g1 = NULL;
    g_object_get (geom, "beam-geometry", &g1, NULL);
    g_print ("\nGEOM1: %s", g1);

    g_object_set (geom, "beam-geometry", "CONE", NULL);
    g_object_get (geom, "beam-geometry", &g1, NULL);
    g_print ("\nGEOM2: %s\n", g1);

    UfoGeom *geom1 = ufo_par_geom_new();
    g_object_get (geom1, "beam-geometry", &g1, NULL);
    g_print ("\nGEOM3: %s", g1);
*/


    return 0;
}

#if 1
void
test_task ()
{
    #define IN_FILE_NAME "/pdv/home/ashkarin/Data/Bug/sino-00177-normalized.tif"

    JsonObject *jgeometry = json_object_new ();
    json_object_set_string_member (jgeometry, "beam-geometry", "parallel");
    json_object_set_double_member (jgeometry, "num-angles", 300);
    json_object_set_double_member (jgeometry, "angle-offset", 0.0);
    json_object_set_double_member (jgeometry, "angle-step", 0.72);
    json_object_set_double_member (jgeometry, "detector-scale", 1.0);
    json_object_set_int_member (jgeometry, "detector-offset", 0);

    JsonObject *jproj_model = json_object_new ();
    json_object_set_string_member (jproj_model, "model", "jOsePh");
    json_object_set_boolean_member (jproj_model, "on-gpu", TRUE);

    JsonObject *jmethod = json_object_new ();
    json_object_set_string_member (jmethod, "plugin", "sart");
    json_object_set_double_member (jmethod, "relaxation-factor", 0.25);
    json_object_set_int_member (jmethod, "max-iterations", 2);

    JsonObject *jprior = json_object_new ();
    json_object_set_string_member (jprior, "object-sparsity", "gradient");
    json_object_set_boolean_member (jprior, "phase-contrast", FALSE);

    GError *error = NULL;
    UfoConfig *config = ufo_config_new();
    GList *path_list = NULL;
    path_list = g_list_append (path_list, "/pdv/home/ashkarin/CurrentWork/ufo.ir/src/");
    ufo_config_add_paths (config, path_list);

    UfoGraph *ufo_task_graph = ufo_task_graph_new();
    UfoScheduler *ufo_scheduler  = ufo_scheduler_new (config, NULL);
    UfoPluginManager *ufo_plugin_manager = ufo_plugin_manager_new (config);

    UfoNode *reader = UFO_NODE (ufo_plugin_manager_get_task (ufo_plugin_manager, "reader", &error));
    UfoNode *writer = UFO_NODE (ufo_plugin_manager_get_task (ufo_plugin_manager, "writer", &error));

    UfoNode *ir_filter = UFO_NODE (ufo_ir_task_new ());

    g_object_set (G_OBJECT (writer),
                  "filename", "out-%05i.tif",
                  NULL);

    g_object_set (G_OBJECT (reader),
                  "path", IN_FILE_NAME,
                  NULL);

/*
    g_object_set (G_OBJECT (ir_filter),
                  "geometry", jgeometry,
                  "prior-knowledge", jprior,
                  "method", jmethod,
                  NULL);
*/
    ufo_task_set_json_object_property (ir_filter, "geometry", jgeometry);
    ufo_task_set_json_object_property (ir_filter, "projector", jproj_model);
    //ufo_task_set_json_object_property (ir_filter, "prior-knowledge", jprior);
    ufo_task_set_json_object_property (ir_filter, "method", jmethod);



    ufo_graph_connect_nodes (ufo_task_graph, reader, ir_filter, NULL);
    ufo_graph_connect_nodes (ufo_task_graph, ir_filter, writer, NULL);

    ufo_base_scheduler_run (ufo_scheduler, UFO_TASK_GRAPH (ufo_task_graph), &error);
    if (error) printf("\nError: Run was unsuccessful: %s\n", error->message);

/*
    g_object_unref (ufo_task_graph);
    g_object_unref (ufo_scheduler);
    g_object_unref (ufo_plugin_manager);

    g_object_unref (writer);
    g_object_unref (reader);
    g_object_unref (ir_filter);*/
}
#endif