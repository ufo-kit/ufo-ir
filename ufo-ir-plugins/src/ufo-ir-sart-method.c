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

#include "ufo-ir-sart-method.h"
#include <math.h>
#include <ufo/ufo.h>

static void ufo_method_interface_init (UfoMethodIface *iface);
static void ufo_copyable_interface_init (UfoCopyableIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrSartMethod, ufo_ir_sart_method, UFO_IR_TYPE_METHOD,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_METHOD,
                                                ufo_method_interface_init)
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_COPYABLE,
                                                ufo_copyable_interface_init))

GQuark
ufo_ir_sart_method_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-sart-method-error-quark");
}

static UfoIrProjectionsSubset *
generate_subsets (UfoIrGeometry *geometry, guint *n_subsets)
{
    gfloat *sin_values = ufo_ir_geometry_scan_angles_host (geometry, SIN_VALUES);
    gfloat *cos_values = ufo_ir_geometry_scan_angles_host (geometry, COS_VALUES);
    guint n_angles = 0;
    g_object_get (geometry, "num-angles", &n_angles, NULL);
    *n_subsets = n_angles;

    UfoIrProjectionsSubset *subsets =
        g_malloc (sizeof (UfoIrProjectionsSubset) * n_angles);

    for (guint i = 0; i < n_angles; ++i) {
        subsets[i].direction = fabs(sin_values[i]) <= fabs(cos_values[i]); // vertical == 1
        subsets[i].offset = i;
        subsets[i].n = 1;
    }

    return subsets;
}

UfoIrMethod *
ufo_ir_sart_method_new (void)
{
    return UFO_IR_METHOD (g_object_new (UFO_IR_TYPE_SART_METHOD, NULL));
}

static void
ufo_ir_sart_method_init (UfoIrSartMethod *self)
{
}

static gboolean
ufo_ir_sart_method_process_real (UfoMethod *method,
                                 UfoBuffer *input,
                                 UfoBuffer *output,
                                 gpointer  pevent)
{
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

    UfoBuffer *sino_tmp   = ufo_buffer_dup (input);
    UfoBuffer *volume_tmp = ufo_buffer_dup (output);

    UfoBuffer *ray_weights = ufo_buffer_dup (input);

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

    //
    // do SART
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

            ufo_op_mul_rows (sino_tmp,
                             ray_weights,
                             sino_tmp,
                             subset[i].offset,
                             subset[i].n,
                             resources,
                             cmd_queue);

            ufo_ir_projector_BP (projector,
                                 output,
                                 sino_tmp,
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
    iface->process = ufo_ir_sart_method_process_real;
}

static UfoCopyable *
ufo_ir_sart_method_copy_real (gpointer origin,
                              gpointer _copy)
{
    UfoCopyable *copy;
    if (_copy)
        copy = UFO_COPYABLE(_copy);
    else
        copy = UFO_COPYABLE (ufo_ir_sart_method_new());

    return copy;
}

static void
ufo_copyable_interface_init (UfoCopyableIface *iface)
{
    iface->copy = ufo_ir_sart_method_copy_real;
}

static void
ufo_ir_sart_method_class_init (UfoIrSartMethodClass *klass)
{
}