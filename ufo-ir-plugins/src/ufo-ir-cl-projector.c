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

#include "ufo-ir-cl-projector.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

static void ufo_copyable_interface_init (UfoCopyableIface *iface);
G_DEFINE_TYPE_WITH_CODE (UfoIrClProjector, ufo_ir_cl_projector, UFO_IR_TYPE_PROJECTOR,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_COPYABLE,
                                                ufo_copyable_interface_init))

#define UFO_IR_CL_PROJECTOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_CL_PROJECTOR, UfoIrClProjectorPrivate))

GQuark
ufo_ir_cl_projector_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-cl-projector-error-quark");
}

struct _UfoIrClProjectorPrivate {
    gchar    *model;
    gpointer fp_kernel[2];
    gpointer bp_kernel;
};

enum {
    PROP_0,
    PROP_MODEL,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoIrProjector *
ufo_ir_cl_projector_new (void)
{
    return UFO_IR_PROJECTOR (g_object_new (UFO_IR_TYPE_CL_PROJECTOR, NULL));
}

static void
ufo_ir_cl_projector_setup_real (UfoProcessor  *projector,
                                UfoResources  *resources,
                                GError        **error)
{
    UFO_PROCESSOR_CLASS (ufo_ir_cl_projector_parent_class)->setup (projector,
                                                                   resources,
                                                                   error);
    if (error && *error)
      return;

    UfoIrClProjectorPrivate *priv = UFO_IR_CL_PROJECTOR_GET_PRIVATE (projector);
    UfoIrGeometry *geometry = NULL;
    gchar *model = NULL;
    gchar *geom = NULL;
    g_object_get (projector, "geometry", &geometry, "model", &model, NULL);
    g_object_get (geometry, "beam-geometry", &geom, NULL);

    gchar *filename = g_strconcat("projector-", geom, "-", model, ".cl", NULL);

    gpointer *kernel[3] = { &priv->bp_kernel,
                            &priv->fp_kernel[Horizontal],
                            &priv->fp_kernel[Vertical]};

    *kernel[0] = ufo_resources_get_kernel (resources, filename, "BP", error);
    if (*error) return;
    *kernel[1] = ufo_resources_get_kernel (resources, filename, "FP_hor", error);
    if (*error) return;
    *kernel[2] = ufo_resources_get_kernel (resources, filename, "FP_vert", error);
    if (*error) return;
}

static void
ufo_ir_cl_projector_set_property (GObject      *object,
                                  guint        property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
    UfoIrClProjectorPrivate *priv = UFO_IR_CL_PROJECTOR_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_MODEL:
            g_free (priv->model);
            // convert model name to lowercase
            priv->model = g_ascii_strdown (g_value_get_string(value), -1);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_cl_projector_get_property (GObject    *object,
                                  guint      property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
    UfoIrClProjectorPrivate *priv = UFO_IR_CL_PROJECTOR_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_MODEL:
            g_value_set_string (value, priv->model);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_cl_projector_dispose (GObject *object)
{
    UfoIrClProjectorPrivate *priv = UFO_IR_CL_PROJECTOR_GET_PRIVATE (object);

    gpointer *kernel[3] = { &priv->bp_kernel,
                            &priv->fp_kernel[Horizontal],
                            &priv->fp_kernel[Vertical]};
    for (int i = 0; i < 3; ++i) {
        UFO_RESOURCES_CHECK_CLERR (clReleaseKernel (*kernel[i]));
        *kernel[i] = NULL;
    }

    G_OBJECT_CLASS (ufo_ir_cl_projector_parent_class)->dispose (object);
}

static void
ufo_ir_cl_projector_finalize (GObject *gobject)
{
    //UfoIrClProjectorPrivate *priv = UFO_IR_CL_PROJECTOR_GET_PRIVATE (gobject);
    G_OBJECT_CLASS(ufo_ir_cl_projector_parent_class)->finalize(gobject);
}

static void
ufo_ir_cl_projector_FP_ROI_real (UfoIrProjector         *projector,
                                 UfoBuffer              *volume,
                                 UfoRegion              *volume_roi,
                                 UfoBuffer              *measurements,
                                 UfoIrProjectionsSubset *subset,
                                 gfloat                 scale,
                                 gpointer               finish_event)
{
    UfoIrClProjectorPrivate *priv = UFO_IR_CL_PROJECTOR_GET_PRIVATE (projector);
    cl_kernel kernel = priv->fp_kernel[subset->direction];
    cl_command_queue cmd_queue = NULL;
    UfoProfiler      *profiler = NULL;
    g_object_get (projector,
                  "ufo-profiler", &profiler,
                  "command-queue", &cmd_queue,
                  NULL);

    cl_mem d_volume = ufo_buffer_get_device_image (volume, cmd_queue);
    cl_mem d_measurements = ufo_buffer_get_device_image (measurements, cmd_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof (cl_mem), &d_volume));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof (UfoRegion), volume_roi));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof (cl_mem), &d_measurements));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 3, sizeof (cl_mem), &d_measurements));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 4, sizeof (gfloat), &scale));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 9, sizeof (UfoIrProjectionsSubset), subset));

    UfoRequisition requisitions;
    ufo_buffer_get_requisition (measurements, &requisitions);
    requisitions.dims[1] = subset->n;

    if (finish_event) {
        UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel(cmd_queue,
                                                          kernel,
                                                          requisitions.n_dims,
                                                          NULL,
                                                          requisitions.dims,
                                                          NULL,
                                                          0,
                                                          NULL,
                                                          (cl_event*)finish_event));
        ufo_profiler_register_event (profiler, cmd_queue, kernel, finish_event);
    } else {
        ufo_profiler_call (profiler, cmd_queue, kernel, requisitions.n_dims,
                           requisitions.dims, NULL);
    }
}

static void
ufo_ir_cl_projector_BP_ROI_real (UfoIrProjector         *projector,
                                 UfoBuffer              *volume,
                                 UfoRegion              *volume_roi,
                                 UfoBuffer              *measurements,
                                 UfoIrProjectionsSubset *subset,
                                 gfloat                 relax_param,
                                 gpointer               finish_event)
{
    UfoIrClProjectorPrivate *priv = UFO_IR_CL_PROJECTOR_GET_PRIVATE (projector);
    cl_kernel kernel = priv->bp_kernel;
    cl_command_queue cmd_queue = NULL;
    UfoProfiler *profiler = NULL;
    g_object_get (projector,
                  "ufo-profiler", &profiler,
                  "command-queue", &cmd_queue,
                  NULL);

    cl_mem d_volume = ufo_buffer_get_device_image (volume, cmd_queue);
    cl_mem d_measurements = ufo_buffer_get_device_image (measurements, cmd_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof (cl_mem), &d_volume));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof (cl_mem), &d_volume));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 2, sizeof (UfoRegion), volume_roi));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 3, sizeof (cl_mem), &d_measurements));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 4, sizeof (gfloat), &relax_param));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 9, sizeof (UfoIrProjectionsSubset), subset));

    UfoRequisition requisitions;
    ufo_buffer_get_requisition (volume, &requisitions);

    if (finish_event) {
        UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel(cmd_queue,
                                                          kernel,
                                                          requisitions.n_dims,
                                                          NULL,
                                                          requisitions.dims,
                                                          NULL,
                                                          0, NULL,
                                                          (cl_event*)finish_event));
        ufo_profiler_register_event (profiler, cmd_queue, kernel, finish_event);
    } else {
        ufo_profiler_call (profiler, cmd_queue, kernel, requisitions.n_dims,
                           requisitions.dims, NULL);
    }
}

static void
ufo_ir_cl_projector_configure_real (UfoProcessor *projector)
{
    UFO_PROCESSOR_CLASS (ufo_ir_cl_projector_parent_class)->configure (projector);
    UfoIrClProjectorPrivate *priv = UFO_IR_CL_PROJECTOR_GET_PRIVATE (projector);

    UfoIrGeometry *geometry = NULL;
    g_object_get (projector, "geometry", &geometry, NULL);

    cl_mem d_sin_vals = ufo_ir_geometry_scan_angles_device (geometry, SIN_VALUES);
    cl_mem d_cos_vals = ufo_ir_geometry_scan_angles_device (geometry, COS_VALUES);

    UfoIrGeometryDims *dims = NULL;
    gsize spec_size = 0;
    gpointer spec = ufo_ir_geometry_get_spec (geometry, &spec_size);
    g_object_get (geometry, "dimensions", &dims, NULL);

    cl_kernel kernel[3] = { priv->bp_kernel,
                            priv->fp_kernel[Horizontal],
                            priv->fp_kernel[Vertical] };

    cl_int err = 0;
    for (int i = 0; i < 3; ++i) {
        err |= clSetKernelArg (kernel[i], 5, sizeof (cl_mem), &d_sin_vals);
        err |= clSetKernelArg (kernel[i], 6, sizeof (cl_mem), &d_cos_vals);
        err |= clSetKernelArg (kernel[i], 7, sizeof (UfoIrGeometryDims), dims);
        err |= clSetKernelArg (kernel[i], 8, spec_size, spec);
        UFO_RESOURCES_CHECK_CLERR (err);
    }
}

static UfoCopyable *
ufo_ir_cl_projector_copy_real (gpointer origin,
                               gpointer _copy)
{
    UfoCopyable *copy;
    if (_copy)
        copy = _copy;
    else
        copy = UFO_COPYABLE (ufo_ir_cl_projector_new());
    UfoIrClProjectorPrivate * priv = UFO_IR_CL_PROJECTOR_GET_PRIVATE (origin);
    g_object_set (G_OBJECT(copy),
                  "model", priv->model,
                  NULL);
    return copy;
}

static void
ufo_copyable_interface_init (UfoCopyableIface *iface)
{
    iface->copy = ufo_ir_cl_projector_copy_real;
}

static void
ufo_ir_cl_projector_class_init (UfoIrClProjectorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = ufo_ir_cl_projector_dispose;
    gobject_class->finalize = ufo_ir_cl_projector_finalize;
    gobject_class->set_property = ufo_ir_cl_projector_set_property;
    gobject_class->get_property = ufo_ir_cl_projector_get_property;

    properties[PROP_MODEL] =
        g_param_spec_string ("model",
                             "The name of the projection model.",
                             "The name of the projection model.",
                             "joseph",
                              G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof(UfoIrClProjectorPrivate));

    UfoIrProjectorClass *projector_class = UFO_IR_PROJECTOR_CLASS(klass);
    projector_class->FP_ROI = ufo_ir_cl_projector_FP_ROI_real;
    projector_class->BP_ROI = ufo_ir_cl_projector_BP_ROI_real;
    UFO_PROCESSOR_CLASS (klass)->setup  = ufo_ir_cl_projector_setup_real;
    UFO_PROCESSOR_CLASS (klass)->configure = ufo_ir_cl_projector_configure_real;
}

static void
ufo_ir_cl_projector_init (UfoIrClProjector *self)
{
    UfoIrClProjectorPrivate *priv = NULL;
    self->priv = priv = UFO_IR_CL_PROJECTOR_GET_PRIVATE (self);
    priv->model = g_strdup ("joseph");
}