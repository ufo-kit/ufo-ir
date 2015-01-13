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

GQuark
ufo_ir_sirt_method_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-sirt-method-error-quark");
}

static UfoIrProjectionsSubset *
generate_subsets (UfoIrGeometry *geometry, guint *n_subsets)
{
    gfloat *sin_values = ufo_ir_geometry_scan_angles_host (geometry, SIN_VALUES);
    gfloat *cos_values = ufo_ir_geometry_scan_angles_host (geometry, COS_VALUES);
    guint n_angles = 0;
    g_object_get (geometry, "num-angles", &n_angles, NULL);

    UfoIrProjectionsSubset *subsets_tmp =
        g_malloc (sizeof (UfoIrProjectionsSubset) * n_angles);

    guint subset_index = 0;
    UfoIrProjectionDirection direction;
    direction = fabs(sin_values[0]) <= fabs(cos_values[0]);
    subsets_tmp[subset_index].direction = direction;
    subsets_tmp[subset_index].offset = 0;
    subsets_tmp[subset_index].n = 1;

    for (guint i = 1; i < n_angles; ++i) {
        direction = fabs(sin_values[i]) <= fabs(cos_values[i]);
        if (direction != subsets_tmp[subset_index].direction) {
            subset_index++;
            subsets_tmp[subset_index].direction = direction;
            subsets_tmp[subset_index].offset = i;
            subsets_tmp[subset_index].n = 1;
        } else {
            subsets_tmp[subset_index].n++;
        }
    }

    *n_subsets = subset_index + 1;
    UfoIrProjectionsSubset *subsets =
        g_memdup (subsets_tmp, sizeof (UfoIrProjectionsSubset) * (*n_subsets));

    g_free (subsets_tmp);

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
}

static gboolean
ufo_ir_sirt_method_process_real (UfoMethod *method,
                                 UfoBuffer *input,
                                 UfoBuffer *output,
                                 gpointer  pevent)
{
    UfoResources   *resources = NULL;
    UfoIrProjector *projector = NULL;
    gpointer       *cmd_queue = NULL;
    gfloat         relax_fact = 0;
    guint          max_iterations = 0;
    g_object_get (method,
                  "ufo-resources",     &resources,
                  "command-queue",     &cmd_queue,
                  "projection-model",  &projector,
                  "relaxation-factor", &relax_fact,
                  "max-iterations",    &max_iterations,
                  NULL);

    UfoIrGeometry *geometry = NULL;
    g_object_get (projector, "geometry", &geometry, NULL);

    UfoBuffer *sino_tmp   = ufo_buffer_dup (input);
    UfoBuffer *volume_tmp = ufo_buffer_dup (output);

    UfoBuffer *pixel_weights = ufo_buffer_dup (output);
    UfoBuffer *ray_weights   = ufo_buffer_dup (input);

    guint n_subsets = 0;
    UfoIrProjectionsSubset *subset = generate_subsets (geometry, &n_subsets);

    //
    // calculate the weighting coefficients
    ufo_op_set (volume_tmp,  1.0f, resources, cmd_queue);
    ufo_op_set (ray_weights, 0.0f, resources, cmd_queue);
    for (guint i = 0 ; i < n_subsets; ++i) {
        ufo_ir_projector_FP (projector,
                             volume_tmp,
                             ray_weights,
                             &subset[i],
                             1.0f, NULL);
    }
    ufo_op_inv (ray_weights, resources, cmd_queue);

    ufo_op_set (sino_tmp,      1.0f, resources, cmd_queue);
    ufo_op_set (pixel_weights, 0.0f, resources, cmd_queue);
    for (guint i = 0 ; i < n_subsets; ++i) {
        ufo_ir_projector_BP (projector,
                             pixel_weights,
                             sino_tmp,
                             &subset[i],
                             relax_fact, NULL);
    }
    ufo_op_inv (pixel_weights, resources, cmd_queue);

    //
    // do SIRT
    guint iteration = 0;
    while (iteration < max_iterations) {
        ufo_buffer_copy (input, sino_tmp);

        for (guint i = 0 ; i < n_subsets; i++) {
            ufo_ir_projector_FP (projector,
                                 output,
                                 sino_tmp,
                                 &subset[i],
                                 -1.0f,
                                 NULL);
        }

        ufo_op_mul (sino_tmp, ray_weights, sino_tmp, resources, cmd_queue);
        ufo_op_set (volume_tmp, 0, resources, cmd_queue);

        for (guint i = 0 ; i < n_subsets; i++) {
            ufo_ir_projector_BP (projector,
                                 volume_tmp,
                                 sino_tmp,
                                 &subset[i],
                                 1.0f,
                                 NULL);
        }
        ufo_op_mul (volume_tmp, pixel_weights, volume_tmp, resources, cmd_queue);
        ufo_op_add (volume_tmp, output, output, resources, cmd_queue);
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
}