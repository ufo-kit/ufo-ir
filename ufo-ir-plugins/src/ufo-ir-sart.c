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

#include "ufo-ir-sart.h"
#include <ufo/ufo.h>
#include <math.h>

static void ufo_method_interface_init (UfoMethodIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrSART, ufo_ir_sart, UFO_TYPE_IR_METHOD,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_METHOD,
                                                ufo_method_interface_init))

#define UFO_IR_SART_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_IR_SART, UfoIrSARTPrivate))

GQuark
ufo_ir_sart_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-sart-error-quark");
}

struct _UfoIrSARTPrivate {
    UfoBuffer *singular_volume;
    UfoBuffer *singular_sino;
    UfoBuffer *ray_weights;
    UfoBuffer *b_temp;
};

static UfoProjectionsSubset *
generate_subsets (UfoGeometry *geometry, guint *n_subsets)
{
    gfloat *sin_values = ufo_geometry_scan_angles_host (geometry, SIN_VALUES);
    gfloat *cos_values = ufo_geometry_scan_angles_host (geometry, COS_VALUES);
    guint n_angles = 0;
    g_object_get (geometry, "num-angles", &n_angles, NULL);
    *n_subsets = n_angles;

    UfoProjectionsSubset *subsets = g_malloc (sizeof (UfoProjectionsSubset) * n_angles);

    for (guint i = 0; i < n_angles; ++i) {
        subsets[i].direction = fabs(sin_values[i]) <= fabs(cos_values[i]); // vertical == 1
        subsets[i].offset = i;
        subsets[i].n = 1;
    }

    return subsets;
}

UfoIrMethod *
ufo_ir_sart_new (void)
{
    UfoIrMethod *method =
        (UfoIrMethod *) g_object_new (UFO_TYPE_IR_SART,
                                      NULL);
    return method;
}

static void
ufo_ir_sart_init (UfoIrSART *self)
{
    UfoIrSARTPrivate *priv = NULL;
    self->priv = priv = UFO_IR_SART_GET_PRIVATE (self);
    priv->singular_volume = NULL;
    priv->singular_sino = NULL;
    priv->ray_weights = NULL;
    priv->b_temp = NULL;
}

static gboolean
ufo_ir_sart_process_real (UfoMethod *method,
                          UfoBuffer *input,
                          UfoBuffer *output)
{
    UfoIrSARTPrivate *priv = UFO_IR_SART_GET_PRIVATE (method);

    UfoResources *resources = NULL;
    UfoProjector *projector = NULL;
    gpointer     *cmd_queue = NULL;
    gfloat        relaxation_factor = 0;
    guint         max_iterations = 0;
    g_object_get (method,
                  "ufo-resources", &resources,
                  "command-queue", &cmd_queue,
                  "projection-model", &projector,
                  "relaxation-factor", &relaxation_factor,
                  "max-iterations", &max_iterations,
                  NULL);

    UfoGeometry *geometry = NULL;
    g_object_get (projector, "geometry", &geometry, NULL);

    //
    // resize
    UfoBuffer **method_buffers[4] = {
        &priv->singular_volume,
        &priv->singular_sino,
        &priv->ray_weights,
        &priv->b_temp
    };
    UfoBuffer *ref_buffers[4] = {
        output,
        input,
        input,
        input
    };
    for (guint i = 0; i < 4; ++i) {
        UfoRequisition _req;
        if (*method_buffers[i]) {
            ufo_buffer_get_requisition (ref_buffers[i], &_req);
            ufo_buffer_resize (*method_buffers[i], &_req);
        } else {
            *method_buffers[i] = ufo_buffer_dup (ref_buffers [i]);
        }
    }
    ufo_op_set (priv->singular_volume, 1.0f, resources, cmd_queue);
    ufo_op_set (priv->singular_sino, 1.0f, resources, cmd_queue);
    ufo_op_set (priv->ray_weights, 0, resources, cmd_queue);

    guint n_subsets = 0;
    UfoProjectionsSubset *subset = generate_subsets (geometry, &n_subsets);

    for (guint i = 0 ; i < n_subsets; ++i) {
        ufo_projector_FP (projector,
                          priv->singular_volume,
                          priv->ray_weights,
                          &subset[i],
                          1.0f,
                          NULL);
    }

    ufo_op_inv (priv->ray_weights, resources, cmd_queue);

    guint iteration = 0;
    while (iteration < max_iterations) {
        ufo_buffer_copy (input, priv->b_temp);
        for (guint i = 0 ; i < n_subsets; i++) {
            ufo_projector_FP (projector,
                              output,
                              priv->b_temp,
                              &subset[i],
                              -1.0f,
                              NULL);

            ufo_op_mul_rows (priv->b_temp,
                             priv->ray_weights,
                             priv->b_temp,
                             subset[i].offset,
                             subset[i].n,
                             resources,
                             cmd_queue);

            ufo_projector_BP (projector,
                              output,
                              priv->b_temp,
                              &subset[i],
                              relaxation_factor,
                              NULL);
        }
        iteration++;
    }

    return TRUE;
}

static void
ufo_method_interface_init (UfoMethodIface *iface)
{
    iface->process = ufo_ir_sart_process_real;
}

static void
ufo_ir_sart_class_init (UfoIrSARTClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    g_type_class_add_private (gobject_class, sizeof(UfoIrSARTPrivate));
}