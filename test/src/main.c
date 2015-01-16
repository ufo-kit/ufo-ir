#include <stdio.h>
#include <glib.h>
#include <ufo/ufo.h>

#include <ufo/ir/ufo-ir.h>

#define IN_FILE_NAME "/pdv/home/ashkarin/Data/ForbildPhantom/forbild512_sino12575.tif"
#define N_ANGLES   144
#define ANGLE_STEP 0.02195f
#define AXIS_POS   -1.0f

/*
#define IN_FILE_NAME "/pdv/home/ashkarin/Data/Bug/bug_sino_2_88(quadro).tif"
#define N_ANGLES   75
#define ANGLE_STEP 0.0504f
#define AXIS_POS   416.0f
*/
int main(int n_args, char *argv[])
{
    GError *error = NULL;
    UfoGraph *ufo_task_graph = ufo_task_graph_new();
    UfoScheduler *ufo_scheduler = ufo_scheduler_new ();
    g_object_set (ufo_scheduler, "enable-tracing", FALSE, NULL);
    UfoPluginManager *manager = ufo_plugin_manager_new ();
    UfoNode *reader = UFO_NODE (ufo_plugin_manager_get_task (manager, "reader", NULL));
    UfoNode *writer = UFO_NODE (ufo_plugin_manager_get_task (manager, "writer", NULL));
    UfoNode *generator = UFO_NODE (ufo_plugin_manager_get_task (manager, "generate", NULL));
    UfoNode *null = UFO_NODE (ufo_plugin_manager_get_task (manager, "null", NULL));
    g_object_set (generator, "width", 729, "height", 729, "bitdepth", 16, NULL);

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

    if (error){
        printf("\nError: Run was unsuccessful: %s\n", error->message);
        return 1;
    }

    g_object_set (geometry,
                  "num-angles", N_ANGLES,
                  "angle-offset", 0.0,
                  "angle-step", ANGLE_STEP,
                  "detector-scale", 1.0,
                  "axis-pos", AXIS_POS,
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
                  "max-iterations", 200,
                  NULL);

    gpointer sirt = ufo_plugin_manager_get_plugin (manager,
                                                   "ufo_ir_sirt_method_new",
                                                   "libufoir_sirt_method.so",
                                                   &error);
    g_print ("\nsirt: %p", sart);
    if (error){
        printf("\nError: Run was unsuccessful: %s\n", error->message);
        return 1;
    }

    g_object_set (sirt,
                  "relaxation-factor", 0.25,
                  "max-iterations", 2000,
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
                  "max-iterations", 200,
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

    //ufo_graph_connect_nodes (ufo_task_graph, reader, null, NULL);
    //ufo_graph_connect_nodes (ufo_task_graph, generator, writer, NULL);
    ufo_graph_connect_nodes (ufo_task_graph, reader, ir, NULL);
    ufo_graph_connect_nodes (ufo_task_graph, ir, writer, NULL);
    ufo_base_scheduler_run (UFO_BASE_SCHEDULER(ufo_scheduler), UFO_TASK_GRAPH (ufo_task_graph), &error);
    if (error){
        printf("\nError: Run was unsuccessful: %s\n", error->message);
        return 1;
    }

    return 0;
}
