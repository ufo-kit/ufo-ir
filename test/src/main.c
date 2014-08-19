#include <stdio.h>
#include <glib.h>
#include <ufo/ufo.h>

#include <ufo/ir/ufo-ir-geometry.h>
#include <ufo/ir/ufo-ir-prior-knowledge.h>
#define IN_FILE_NAME "/pdv/home/ashkarin/Data/ForbildPhantom/forbild512_sino06288.tif"

int main(int n_args, char *argv[])
{
    GError *error = NULL;
    UfoConfig *config = ufo_config_new();
    UfoGraph *ufo_task_graph = ufo_task_graph_new();
    UfoScheduler *ufo_scheduler = ufo_scheduler_new (config, NULL);
    //g_object_set (ufo_scheduler, "enable-tracing", TRUE, NULL);
    UfoPluginManager *manager = ufo_plugin_manager_new (config);
    UfoNode *reader = UFO_NODE (ufo_plugin_manager_get_task (manager, "reader", NULL));
    UfoNode *writer = UFO_NODE (ufo_plugin_manager_get_task (manager, "writer", NULL));
    UfoNode *ir = UFO_NODE (ufo_plugin_manager_get_task (manager, "ir", &error));
    if (error){
        printf("\nError: Run was unsuccessful: %s\n", error->message);
        return 1;
    }

    g_object_set (G_OBJECT (writer), "filename", "out-%05i.tif", NULL);
    g_object_set (G_OBJECT (reader), "path", IN_FILE_NAME, NULL);

    gpointer geometry = ufo_plugin_manager_get_plugin (manager,
                                                       "ufo_ir_parallel_geometry_new",
                                                       "libufoir_parallel_geometry.so",
                                                       &error);

    g_print ("\ngeometry: %p %d %p", geometry, UFO_IR_IS_GEOMETRY (geometry), error);
    if (error){
        printf("\nError: Run was unsuccessful: %s\n", error->message);
        return 1;
    }

    g_object_set (geometry,
                  "num-angles", 287,
                  "angle-offset", 0.0,
                  "angle-step", 0.6288,
                  "detector-scale", 1.0,
                  "detector-offset", 0,
                  NULL);


    gpointer projector = ufo_plugin_manager_get_plugin (manager,
                                                        "ufo_ir_cl_projector_new",
                                                        "libufoir_cl_projector.so",
                                                        &error);
    g_print ("\nprojector: %p", projector);
    if (error){
        printf("\nError: Run was unsuccessful: %s\n", error->message);
        return 1;
    }

    g_object_set (projector,
                  "model", "Joseph",
                  NULL);

    gpointer sart = ufo_plugin_manager_get_plugin (manager,
                                                   "ufo_ir_sart_method_new",
                                                   "libufoir_sart_method.so",
                                                   &error);
    g_print ("\nsart: %p", sart);
    if (error){
        printf("\nError: Run was unsuccessful: %s\n", error->message);
        return 1;
    }

    g_object_set (sart,
                  "relaxation-factor", 0.25,
                  "max-iterations", 300,
                  NULL);

    gpointer asdpocs = ufo_plugin_manager_get_plugin (manager,
                                                     "ufo_ir_asdpocs_method_new",
                                                     "libufoir_asdpocs_method.so",
                                                     &error);
    g_print ("\nasdpocs: %p", asdpocs);
    if (error){
        printf("\nError: Run was unsuccessful: %s\n", error->message);
        return 1;
    }

    g_object_set (asdpocs,
                  "df-minimizer", sart,
                  "max-iterations", 10,
                  NULL);

    gpointer sparsity = ufo_plugin_manager_get_plugin (manager,
                                                       "ufo_ir_gradient_sparsity_new",
                                                       "libufoir_gradient_sparsity.so",
                                                       &error);

    g_print ("\nsparsity: %p", sparsity);
    if (error){
        printf("\nError: Run was unsuccessful: %s\n", error->message);
        return 1;
    }

    UfoIrPriorKnowledge *prior = ufo_ir_prior_knowledge_new ();
    ufo_ir_prior_knowledge_set_pointer (prior, "image-sparsity", sparsity);

    g_object_set (ir,
                  "method", sart,
                  "geometry", geometry,
                  "projector", projector,
                  "prior-knowledge", prior,
                  NULL);

    ufo_graph_connect_nodes (ufo_task_graph, reader, ir, NULL);
    ufo_graph_connect_nodes (ufo_task_graph, ir, writer, NULL);
    ufo_base_scheduler_run (ufo_scheduler, UFO_TASK_GRAPH (ufo_task_graph), &error);
    if (error){
        printf("\nError: Run was unsuccessful: %s\n", error->message);
        return 1;
    }

    return 0;
}