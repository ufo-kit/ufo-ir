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

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "ufo-ir-method-task-node.h"
#include <ufo/ufo.h>

// Private methods definitions
// Class related methods
static void ufo_ir_method_task_node_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ufo_ir_method_task_node_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ufo_ir_method_task_node_finalize (GObject *object);
static void ufo_ir_method_task_node_dispose (GObject *object);

// UfoTask Interface related methods
static void ufo_task_interface_init (UfoTaskIface *iface);
static void ufo_ir_method_task_node_setup (UfoTask *task, UfoResources *resources, GError **error);
static guint ufo_ir_method_task_node_get_num_inputs (UfoTask *task);
static guint ufo_ir_method_task_node_get_num_dimensions (UfoTask *task, guint input);
static UfoTaskMode ufo_ir_method_task_node_get_mode (UfoTask *task);
static void ufo_ir_method_task_node_get_requisition (UfoTask *task, UfoBuffer **inputs, UfoRequisition *requisition);
static void ufo_ir_method_task_node_set_json_object_property (UfoTask *task, const gchar *prop_name, JsonObject *object);

// UfoTaskNode class related methods
// UfoNode *ufo_ir_method_task_node_copy (UfoNode *node, GError **error);

// IrMethod private Methods
static void ufo_ir_method_task_node_setup_projection_model(UfoIrMethodTaskNode *method);
// -----------------------------------------------------------------------------

G_DEFINE_TYPE_WITH_CODE (UfoIrMethodTaskNode, ufo_ir_method_task_node, UFO_TYPE_TASK_NODE,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK, ufo_task_interface_init))

#define UFO_IR_METHOD_TASK_NODE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_METHOD_TASK_NODE, UfoIrMethodTaskNodePrivate))

struct _UfoIrMethodTaskNodePrivate {
    UfoResources     *resources;
    cl_command_queue cmd_queue;
    gchar *projection_model_name;
    UfoIrProjectorTaskNode *projection_model;
    guint iterations_count;
};

enum {
    PROP_0,
    PROP_PROJECTION_MODEL,
    PROP_ITERATIONS_COUNT,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = {NULL, };

static void
ufo_ir_method_task_node_class_init (UfoIrMethodTaskNodeClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->set_property = ufo_ir_method_task_node_set_property;
    gobject_class->get_property = ufo_ir_method_task_node_get_property;
    gobject_class->finalize     = ufo_ir_method_task_node_finalize;
    gobject_class->dispose      = ufo_ir_method_task_node_dispose;

    properties[PROP_ITERATIONS_COUNT] =
            g_param_spec_uint("iterations_count",
                              "Number of iterations of method",
                              "Number of iterations of method",
                              1,
                              G_MAXUINT,
                              1,
                              G_PARAM_READWRITE);
    properties[PROP_PROJECTION_MODEL] =
            g_param_spec_string("projection_model",
                                "Current projection model",
                                "Current projection model",
                                "",
                                G_PARAM_READWRITE);
    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++){
        g_object_class_install_property (gobject_class, i, properties[i]);
    }

    g_type_class_add_private (gobject_class, sizeof(UfoIrMethodTaskNodePrivate));

    // TODO: Is it really needed?
    //UFO_NODE_CLASS (klass)->copy = ufo_ir_method_task_node_copy;
}

static void
ufo_task_interface_init (UfoTaskIface *iface)
{
    iface->setup = ufo_ir_method_task_node_setup;
    iface->get_num_inputs = ufo_ir_method_task_node_get_num_inputs;
    iface->get_num_dimensions = ufo_ir_method_task_node_get_num_dimensions;
    iface->get_mode = ufo_ir_method_task_node_get_mode;
    iface->get_requisition = ufo_ir_method_task_node_get_requisition;
    iface->set_json_object_property = ufo_ir_method_task_node_set_json_object_property;
}

static void
ufo_ir_method_task_node_init(UfoIrMethodTaskNode *self)
{
    UfoIrMethodTaskNodePrivate *priv = NULL;
    self->priv = priv = UFO_IR_METHOD_TASK_NODE_GET_PRIVATE(self);
    priv->resources = NULL;
    priv->cmd_queue = NULL;
    priv->projection_model_name = g_strdup("");
    priv->iterations_count = 10;
}

UfoNode *
ufo_ir_method_task_node_new (void)
{
    return UFO_NODE (g_object_new (UFO_IR_TYPE_METHOD_TASK_NODE, NULL));
}

// -----------------------------------------------------------------------------
// Private methods realizations
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Class related methods
// -----------------------------------------------------------------------------
static void
ufo_ir_method_task_node_set_property (GObject      *object,
                                      guint        property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
    UfoIrMethodTaskNodePrivate *priv = UFO_IR_METHOD_TASK_NODE_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_ITERATIONS_COUNT:
            priv->iterations_count = g_value_get_uint(value);
            break;
        case PROP_PROJECTION_MODEL:
            g_free(priv->projection_model_name);
            priv->projection_model_name = g_value_dup_string(value);
            ufo_ir_method_task_node_setup_projection_model(UFO_IR_METHOD_TASK_NODE(object));
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_method_task_node_get_property (GObject    *object,
                                      guint      property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
    UfoIrMethodTaskNodePrivate *priv = UFO_IR_METHOD_TASK_NODE_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_ITERATIONS_COUNT:
            g_value_set_uint(value, priv->iterations_count);
            break;
        case PROP_PROJECTION_MODEL:
            g_value_set_string(value, priv->projection_model_name);
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_method_task_node_finalize (GObject *object)
{
    UfoIrMethodTaskNodePrivate *priv = UFO_IR_METHOD_TASK_NODE_GET_PRIVATE (object);
    if (priv->cmd_queue){
        UFO_RESOURCES_CHECK_CLERR (clReleaseCommandQueue(priv->cmd_queue));
    }

    G_OBJECT_CLASS (ufo_ir_method_task_node_parent_class)->finalize (object);
}

static void
ufo_ir_method_task_node_dispose (GObject *object)
{
    UfoIrMethodTaskNodePrivate *priv = UFO_IR_METHOD_TASK_NODE_GET_PRIVATE (object);

    // TODO: Check object to clear
    g_clear_object (&priv->projection_model_name);
    g_clear_object (&priv->projection_model);

    G_OBJECT_CLASS (ufo_ir_method_task_node_parent_class)->dispose (object);
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// UfoTask Interface related methods
// -----------------------------------------------------------------------------
static void
ufo_ir_method_task_node_setup (UfoTask      *task,
                               UfoResources *resources,
                               GError       **error)
{
    UfoIrMethodTaskNodePrivate *priv = UFO_IR_METHOD_TASK_NODE_GET_PRIVATE (task);
    UfoGpuNode *node = UFO_GPU_NODE (ufo_task_node_get_proc_node (UFO_TASK_NODE (task)));

    priv->resources = g_object_ref (resources);
    priv->cmd_queue = ufo_gpu_node_get_cmd_queue (node);
    UFO_RESOURCES_CHECK_CLERR (clRetainCommandQueue (priv->cmd_queue));

    UfoProfiler *profiler = ufo_task_node_get_profiler (UFO_TASK_NODE (task));

//    g_object_set (priv->projector,
//                  "ufo-profiler", profiler,
//                  "geometry", priv->geometry,
//                  "command-queue", priv->cmd_queue, NULL);

//    g_object_set (priv->method,
//                  "projection-model", priv->projector,
//                  "command-queue", priv->cmd_queue,
//                  "ufo-profiler", profiler, NULL);

//    if (priv->prior)
//        g_object_set (priv->method, "prior-knowledge", priv->prior, NULL);

//    ufo_ir_geometry_setup  (priv->geometry, priv->resources, error);

//    if (error && *error)
//      return;

//    ufo_processor_setup (UFO_PROCESSOR (priv->projector), priv->resources, error);

//    if (error && *error)
//      return;

//    ufo_processor_setup (UFO_PROCESSOR (priv->method), priv->resources, error);
}

static guint
ufo_ir_method_task_node_get_num_inputs (UfoTask *task)
{
    return 1;
}

static guint
ufo_ir_method_task_node_get_num_dimensions (UfoTask *task,
                                            guint   input)
{
    g_return_val_if_fail (input == 0, 0);
    return 2;
}

static UfoTaskMode
ufo_ir_method_task_node_get_mode (UfoTask *task)
{
    return UFO_TASK_MODE_PROCESSOR | UFO_TASK_MODE_GPU;
}

static void
ufo_ir_method_task_node_get_requisition (UfoTask        *task,
                                         UfoBuffer      **inputs,
                                         UfoRequisition *requisition)
{
    UfoIrMethodTaskNodePrivate *priv = UFO_IR_METHOD_TASK_NODE_GET_PRIVATE (task);

    if(priv->projection_model == NULL)
    {
        g_error("Projcetion model not defined");
    }
    else
    {
        ufo_task_get_requisition(UFO_TASK(priv->projection_model), inputs, requisition);
    }

    // TODO: What is it?
    //ufo_processor_configure (UFO_PROCESSOR (priv->projector));
}

static void
ufo_ir_method_task_node_set_json_object_property (UfoTask     *task,
                                                  const gchar *prop_name,
                                                  JsonObject  *json_obj)
{

    UfoIrMethodTaskNodePrivate *priv = UFO_IR_METHOD_TASK_NODE_GET_PRIVATE (task);
    gpointer obj = NULL;

    // TODO: realize
    g_error("ufo_ir_method_task_node_set_json_object_property not implemented");

//    if (g_strcmp0 (prop_name, "geometry") == 0) {
//        obj = ufo_ir_geometry_from_json (json_obj, priv->plugin_manager);
//    }
//    else if (g_strcmp0 (prop_name, "projector") == 0) {
//        obj = ufo_ir_projector_from_json (json_obj, priv->plugin_manager);
//    }
//    else if (g_strcmp0 (prop_name, "method") == 0) {
//        obj = ufo_object_from_json (json_obj, priv->plugin_manager);
//    }
//    else if (g_strcmp0 (prop_name, "prior-knowledge") == 0) {
//        obj = ufo_ir_prior_knowledge_from_json (json_obj, priv->plugin_manager);
//    }

    g_object_set (task, prop_name, obj, NULL);
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// UfoTaskNode class related methods
// -----------------------------------------------------------------------------
//static UfoNode *
//ufo_ir_method_task_node_copy (UfoNode *node,
//                              GError **error)
//{
//    //UfoNode *copy = UFO_NODE (ufo_ir_task_new());
//    //UfoIrTaskPrivate *priv = UFO_IR_TASK_GET_PRIVATE (node);

////    UfoIrMethod *copy_method = UFO_IR_METHOD (ufo_copyable_copy (priv->method, NULL));

////    if (copy_method) {
////        g_object_set (G_OBJECT (copy), "method", copy_method, NULL);
////        g_object_unref (copy_method);
////    }

////    UfoIrProjector *copy_projector = UFO_IR_PROJECTOR (ufo_copyable_copy (priv->projector, NULL));

////    if (copy_projector) {
////        g_object_set (G_OBJECT (copy), "projector", copy_projector, NULL);
////        g_object_unref (copy_projector);
////    }

////    UfoIrGeometry *copy_geometry = UFO_IR_GEOMETRY (ufo_copyable_copy (priv->geometry, NULL));

////    if (copy_geometry) {
////        g_object_set (G_OBJECT (copy), "geometry", copy_geometry, NULL);
////        g_object_unref (copy_geometry);
////    }

////    UfoIrPriorKnowledge *copy_prior = ufo_ir_prior_knowledge_copy (priv->prior);

////    if (copy_prior) {
////        g_object_set (G_OBJECT (copy), "prior-knowledge", copy_prior, NULL);
////        ufo_ir_prior_knowledge_unref (copy_prior);
////    }

//    return copy;
//}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------


static void ufo_ir_method_task_node_setup_projection_model(UfoIrMethodTaskNode *method)
{
    // TODO: Implement projection model setup
    g_error("ufo_ir_method_task_node_set_json_object_property not implemented");
}
