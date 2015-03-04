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

#include "ufo-ir-cgls-method.h"
#include <math.h>
#include <ufo/ufo.h>

static void ufo_method_interface_init (UfoMethodIface *iface);
static void ufo_copyable_interface_init (UfoCopyableIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrCglsMethod, ufo_ir_cgls_method, UFO_IR_TYPE_METHOD,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_METHOD,
                                                ufo_method_interface_init)
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_COPYABLE,
                                                ufo_copyable_interface_init))

GQuark
ufo_ir_cgls_method_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-cgls-method-error-quark");
}

static gfloat
l2_norm(UfoBuffer *arg,
        UfoResources *resources,
        gpointer command_queue)
{
    UfoRequisition arg_requisition;
    ufo_buffer_get_requisition (arg, &arg_requisition);

    gfloat *values = ufo_buffer_get_host_array (arg, command_queue);

    guint length = 0;
    for(guint dimension = 0; dimension < arg_requisition.n_dims; dimension++)
    {
        length += (guint)arg_requisition.dims[dimension];
    }

    gfloat norm = 0;
    for (guint i = 0; i < length; ++i)
    {
        norm += powf (values[i], 2);
    }

    norm = sqrtf(norm);
    return norm;
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
ufo_ir_cgls_method_new (void)
{
    return UFO_IR_METHOD (g_object_new (UFO_IR_TYPE_CGLS_METHOD, NULL));
}

static void
ufo_ir_cgls_method_init (UfoIrCglsMethod *self)
{
}

static gboolean
ufo_ir_cgls_method_process_real (UfoMethod *method,
                                 UfoBuffer *input,
                                 UfoBuffer *output,
                                 gpointer  pevent)
{
    UfoResources   *resources = NULL;
    UfoIrProjector *projector = NULL;
    gpointer       *cmd_queue = NULL;
    gfloat         shift = 0;
    guint          max_iterations = 0;
    g_object_get (method,
                  "ufo-resources",     &resources,
                  "command-queue",     &cmd_queue,
                  "projection-model",  &projector,
                  "max-iterations",    &max_iterations,
                  NULL);

    UfoIrGeometry *geometry = NULL;
    g_object_get (projector, "geometry", &geometry, NULL);

    guint n_subsets = 0;
    UfoIrProjectionsSubset *subset = generate_subsets (geometry, &n_subsets);

    // x = zeros(n, 1)
    ufo_op_set(output, 0.0f, resources, cmd_queue);

    // r = b - A * x
    UfoBuffer *r = ufo_buffer_dup(input);
    ufo_buffer_copy(input, r);
    for (guint i = 0 ; i < n_subsets; i++)
    {
        ufo_ir_projector_FP (projector, output, r, &subset[i], -1.0f,NULL);
    }

    // s = A' * r - shift * x
    UfoBuffer *s = ufo_buffer_dup(output);
    UfoBuffer *tempBp = ufo_buffer_dup(output); // tempBp will be used in main loop too
    ufo_op_set(tempBp, 0.0f, resources, cmd_queue);
    ufo_op_set(s, 0.0f, resources, cmd_queue);
    for (guint i = 0 ; i < n_subsets; i++)
    {
        ufo_ir_projector_BP (projector, tempBp, r, &subset[i], 1.0f, NULL);
    }
    ufo_op_add2(tempBp, output, -1.0f * shift, s, resources, cmd_queue);

    // Initialize

    // p = s
    UfoBuffer * p = ufo_buffer_dup(s);
    ufo_buffer_copy(s, p);

    // norms0 = norm(s)
    gfloat norms0 = l2_norm(s, resources, cmd_queue);

    // gamma = normso^2
    gfloat gamma = norms0 * norms0;

    // Main loop
    for(guint iterationNum = 0; iterationNum < max_iterations; ++iterationNum)
    {
        // q = A * p
        UfoBuffer *q = ufo_buffer_dup(output);
        ufo_op_set(q, 0.0f, resources, cmd_queue);
        for (guint i = 0 ; i < n_subsets; i++)
        {
            ufo_ir_projector_FP (projector, p, q, &subset[i], 1.0f,NULL);
        }

        // delta = norm(q)^2 + shift * norm(p) ^ 2
        gfloat normq = l2_norm(q, resources, cmd_queue);
        gfloat normp = l2_norm(p, resources, cmd_queue);
        gfloat delta = normq * normq + shift * normp * normp;
        if(delta == 0)
        {
            delta = 1E-06;
        }

        // alpha = gamma / delta
        gfloat alpha = gamma / delta;

        // x = x + alpha * p;
        ufo_op_add2(output, p, alpha, output, resources, cmd_queue);

        // r = r - alpha * q
        ufo_op_add2(r, q, -1.0f * alpha, r, resources, cmd_queue);

        // s = A' * r - shift * x;
        ufo_op_set(tempBp, 0.0f, resources, cmd_queue);
        ufo_op_set(s, 0.0f, resources, cmd_queue);
        for (guint i = 0 ; i < n_subsets; i++)
        {
            ufo_ir_projector_BP (projector, tempBp, r, &subset[i],1.0f,NULL);
        }
        ufo_op_add2(tempBp, output, -1.0f * shift, s, resources, cmd_queue);

        gfloat norms = l2_norm(s, resources, cmd_queue);
        gfloat gamma1 = gamma;
        gamma = norms * norms;
        gfloat beta = gamma / gamma1;
        ufo_op_add2(s, p, beta, p, resources, cmd_queue);
    }

    return TRUE;
}

static void
ufo_method_interface_init (UfoMethodIface *iface)
{
    iface->process = ufo_ir_cgls_method_process_real;
}

static UfoCopyable *
ufo_ir_cgls_method_copy_real (gpointer origin,
                              gpointer _copy)
{
    UfoCopyable *copy;
    if (_copy)
        copy = UFO_COPYABLE(_copy);
    else
        copy = UFO_COPYABLE (ufo_ir_cgls_method_new());

    return copy;
}

static void
ufo_copyable_interface_init (UfoCopyableIface *iface)
{
    iface->copy = ufo_ir_cgls_method_copy_real;
}

static void
ufo_ir_cgls_method_class_init (UfoIrCglsMethodClass *klass)
{
}
