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

#include "ufo-ir-sirt-method.h"
#include <math.h>
#include <ufo/ufo.h>

static void ufo_method_interface_init (UfoMethodIface *iface);
static void ufo_copyable_interface_init (UfoCopyableIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrSirtMethod, ufo_ir_sirt_method, UFO_IR_TYPE_METHOD,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_METHOD,
                                                ufo_method_interface_init)
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_COPYABLE,
                                                ufo_copyable_interface_init))

#define UFO_IR_SIRT_METHOD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_SIRT_METHOD, UfoIrSirtMethodPrivate))

GQuark
ufo_ir_sirt_method_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-sirt-method-error-quark");
}

struct _UfoIrSirtMethodPrivate {
    UfoBuffer *singular_volume;
    UfoBuffer *singular_sino;
    UfoBuffer *ray_weights;
    UfoBuffer *pixel_weights;
    UfoBuffer *b_temp;
};

static UfoIrProjectionsSubset *
generate_subsets (UfoIrGeometry *geometry, guint *n_subsets)
{
    gfloat *sin_values = ufo_ir_geometry_scan_angles_host (geometry, SIN_VALUES);
    gfloat *cos_values = ufo_ir_geometry_scan_angles_host (geometry, COS_VALUES);

    // 1. calculate the number of subsets
    *n_subsets = 0;

    UfoIrProjectionsSubset *subsets =
        g_malloc (sizeof (UfoIrProjectionsSubset) * (*n_subsets));

    // 2. create subsets
/*
    for (guint i = 0; i < n_angles; ++i) {
        subsets[i].direction = fabs(sin_values[i]) <= fabs(cos_values[i]); // vertical == 1
        subsets[i].offset = i;
        subsets[i].n = 1;
    }
*/

    return subsets;
}

UfoIrMethod *
ufo_ir_sirt_method_new (void)
{
    return UFO_IR_METHOD (g_object_new (UFO_IR_TYPE_SIRT_METHOD, NULL));
}

static void
ufo_ir_sirt_method_init (UfoIrSirtMethod *self)
{
    UfoIrSirtMethodPrivate *priv = NULL;
    self->priv = priv = UFO_IR_SIRT_METHOD_GET_PRIVATE (self);
    priv->singular_volume = NULL;
    priv->singular_sino = NULL;
    priv->ray_weights = NULL;
    priv->b_temp = NULL;
}

static gboolean
ufo_ir_sirt_method_process_real (UfoMethod *method,
                                 UfoBuffer *input,
                                 UfoBuffer *output,
                                 gpointer  pevent)
{
    UfoIrSirtMethodPrivate *priv = UFO_IR_SIRT_METHOD_GET_PRIVATE (method);

    UfoResources   *resources = NULL;
    UfoIrProjector *projector = NULL;
    gpointer       *cmd_queue = NULL;
    gfloat         relaxation_factor = 0;
    guint          max_iterations = 0;
    g_object_get (method,
                  "ufo-resources", &resources,
                  "command-queue", &cmd_queue,
                  "projection-model", &projector,
                  "relaxation-factor", &relaxation_factor,
                  "max-iterations", &max_iterations,
                  NULL);

    UfoIrGeometry *geometry = NULL;
    g_object_get (projector, "geometry", &geometry, NULL);

    //
    // resize
    const int n_buffers = 4;
    UfoBuffer **method_buffers[n_buffers] = {
        &priv->singular_volume,
        &priv->singular_sino,
        &priv->ray_weights,
        &priv->b_temp
    };
    UfoBuffer *ref_buffers[n_buffers] = {
        output,
        input,
        input,
        input
    };
    for (guint i = 0; i < n_buffers; ++i) {
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
    UfoIrProjectionsSubset *subset = generate_subsets (geometry, &n_subsets);

    for (guint i = 0 ; i < n_subsets; ++i) {
        ufo_ir_projector_FP (projector,
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
            ufo_ir_projector_FP (projector,
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
        }
        for (guint i = 0 ; i < n_subsets; i++) {
            ufo_ir_projector_BP (projector,
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
    iface->process = ufo_ir_sirt_method_process_real;
}

static UfoCopyable *
ufo_ir_sirt_method_copy_real (gpointer origin,
                              gpointer _copy)
{
    UfoCopyable *copy;
    if (_copy)
        copy = UFO_COPYABLE(_copy);
    else
        copy = UFO_COPYABLE (ufo_ir_sirt_method_new());

    return copy;
}

static void
ufo_copyable_interface_init (UfoCopyableIface *iface)
{
    iface->copy = ufo_ir_sirt_method_copy_real;
}

static void
ufo_ir_sirt_method_class_init (UfoIrSirtMethodClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    g_type_class_add_private (gobject_class, sizeof(UfoIrSirtMethodPrivate));
}