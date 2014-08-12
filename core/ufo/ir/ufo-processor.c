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

#include <ufo/ir/ufo-processor.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

G_DEFINE_TYPE (UfoProcessor, ufo_processor, G_TYPE_OBJECT)

#define UFO_PROCESSOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_PROCESSOR, UfoProcessorPrivate))

struct _UfoProcessorPrivate {
    UfoResources *resources;
    UfoProfiler  *profiler;
    gpointer      cmd_queue;
};

enum {
    PROP_0,
    PROP_UFO_RESOURCES,
    PROP_UFO_PROFILER,
    PROP_CL_COMMAND_QUEUE,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoProcessor *
ufo_processor_new (void)
{
    return (UfoProcessor *) g_object_new (UFO_TYPE_PROCESSOR,
                                       NULL);
}

static void
ufo_processor_init (UfoProcessor *self)
{
    UfoProcessorPrivate *priv = NULL;
    self->priv = priv = UFO_PROCESSOR_GET_PRIVATE (self);
    priv->resources = NULL;
    priv->profiler  = NULL;
    priv->cmd_queue = NULL;
}

static void
ufo_processor_set_property (GObject      *object,
                            guint        property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    UfoProcessorPrivate *priv = UFO_PROCESSOR_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_UFO_RESOURCES:
            g_clear_object(&priv->resources);
            priv->resources = g_object_ref (g_value_get_pointer (value));
            break;
        case PROP_UFO_PROFILER:
            g_clear_object(&priv->profiler);
            priv->profiler = g_object_ref (g_value_get_pointer (value));
            break;
        case PROP_CL_COMMAND_QUEUE:
            if (priv->cmd_queue) {
                 UFO_RESOURCES_CHECK_CLERR (clReleaseCommandQueue (priv->cmd_queue));
            }
            priv->cmd_queue = g_value_get_pointer (value);
            UFO_RESOURCES_CHECK_CLERR (clRetainCommandQueue (priv->cmd_queue));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_processor_get_property (GObject    *object,
                            guint      property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
    UfoProcessorPrivate *priv = UFO_PROCESSOR_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_UFO_RESOURCES:
            g_value_set_pointer (value, priv->resources);
            break;
        case PROP_UFO_PROFILER:
            g_value_set_pointer (value, priv->profiler);
            break;
        case PROP_CL_COMMAND_QUEUE:
            g_value_set_pointer (value, priv->cmd_queue);
            UFO_RESOURCES_CHECK_CLERR (clRetainCommandQueue (priv->cmd_queue));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_processor_dispose (GObject *object)
{
    UfoProcessorPrivate *priv = UFO_PROCESSOR_GET_PRIVATE (object);
    g_clear_object(&priv->resources);
    g_clear_object(&priv->profiler);

    if (priv->cmd_queue) {
        UFO_RESOURCES_CHECK_CLERR (clReleaseCommandQueue (priv->cmd_queue));
        priv->cmd_queue = NULL;
    }

    G_OBJECT_CLASS (ufo_processor_parent_class)->dispose (object);
}

static void
ufo_processor_finalize (GObject *object)
{
    //UfoProcessorPrivate *priv = UFO_PROCESSOR_GET_PRIVATE (object);
    G_OBJECT_CLASS (ufo_processor_parent_class)->finalize (object);
}

static void
ufo_processor_setup_real (UfoProcessor *processor,
                          UfoResources *resources,
                          GError       **error)
{
    g_object_set (processor,
                  "ufo-resources", resources,
                  NULL);
}

static void
ufo_processor_class_init (UfoProcessorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = ufo_processor_finalize;
    gobject_class->dispose = ufo_processor_dispose;
    gobject_class->set_property = ufo_processor_set_property;
    gobject_class->get_property = ufo_processor_get_property;

    properties[PROP_UFO_RESOURCES] =
        g_param_spec_pointer("ufo-resources",
                             "Pointer to the instance of UfoResources.",
                             "Pointer to the instance of UfoResources.",
                             G_PARAM_READWRITE);

    properties[PROP_UFO_PROFILER] =
        g_param_spec_pointer("ufo-profiler",
                             "Pointer to the instance of UfoProfiler.",
                             "Pointer to the instance of UfoProfiler.",
                             G_PARAM_READWRITE);

    properties[PROP_CL_COMMAND_QUEUE] =
        g_param_spec_pointer("command-queue",
                             "Pointer to the instance of cl_command_queue.",
                             "Pointer to the instance of cl_command_queue.",
                             G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof (UfoProcessorPrivate));
    klass->setup = ufo_processor_setup_real;
}

void
ufo_processor_setup (UfoProcessor *processor,
                     UfoResources *resources,
                     GError       **error)
{
    g_return_if_fail(UFO_IS_PROCESSOR (processor) &&
                     UFO_IS_RESOURCES (resources));

    UfoProcessorClass *klass = UFO_PROCESSOR_GET_CLASS (processor);
    g_object_set (processor, "ufo-resources", resources, NULL);
    klass->setup (processor, resources, error);
}