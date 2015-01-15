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

#include "ufo-ir-linearized-bregman-method.h"
#include <ufo/ufo.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

static void ufo_method_interface_init (UfoMethodIface *iface);
static void ufo_copyable_interface_init (UfoCopyableIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrLinearizedBregmanMethod, ufo_ir_linearized_bregman_method, UFO_IR_TYPE_METHOD,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_METHOD,
                                                ufo_method_interface_init)
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_COPYABLE,
                                                ufo_copyable_interface_init))

#define UFO_IR_LINEARIZED_BREGMAN_METHOD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_LINEARIZED_BREGMAN_METHOD, UfoIrLinearizedBregmanMethodPrivate))

GQuark
ufo_ir_linearized_bregman_method_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-linearized-bregman-method-error-quark");
}

struct _UfoIrLinearizedBregmanMethodPrivate {
    UfoMethod *df_minimizer;
    UfoMethod *shrinkage;

    gpointer  gradient_kernel;
    gpointer  Ix_kernel;

    guint  n_lb_iterations;
    gfloat shrinkage_psi;
};

enum {
    PROP_0 = 0,
    PROP_DF_MINIMIZER,
    PROP_N_LB_ITERATIONS,
    PROP_SHRINKAGE_PSI,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoIrMethod *
ufo_ir_linearized_bregman_method_new (void)
{
    return UFO_IR_METHOD (g_object_new (UFO_IR_TYPE_LINEARIZED_BREGMAN_METHOD, NULL));
}

static void
ufo_ir_linearized_bregman_method_init (UfoIrLinearizedBregmanMethod *self)
{
    UfoIrLinearizedBregmanMethodPrivate *priv = NULL;
    self->priv = priv = UFO_IR_LINEARIZED_BREGMAN_METHOD_GET_PRIVATE (self);
    priv->df_minimizer = NULL;
    priv->gradient_kernel = NULL;
    priv->Ix_kernel = NULL;
    priv->shrinkage_psi = 0.5;
    priv->n_lb_iterations = 20;

    GError *error = NULL;
    UfoPluginManager *plugin_manager = ufo_plugin_manager_new ();
    priv->shrinkage = UFO_METHOD (ufo_plugin_manager_get_plugin (plugin_manager,
                                                  "ufo_math_shrinkage_method_new",
                                                  "libufo_math_shrinkage_method.so",
                                                   &error));

    if (error){
        g_error("\nError: Linearized Bregman method was not initialized: %s\n", error->message);
    }
}

static void
ufo_ir_linearized_bregman_method_set_property (GObject      *object,
                                               guint        property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec)
{
    UfoIrLinearizedBregmanMethodPrivate *priv =
        UFO_IR_LINEARIZED_BREGMAN_METHOD_GET_PRIVATE (object);

    GObject *value_object;

    switch (property_id) {
        case PROP_DF_MINIMIZER:
            {
                value_object = g_value_get_object (value);

                if (priv->df_minimizer) {
                    g_object_unref (priv->df_minimizer);
                }
                if (value_object != NULL) {
                    priv->df_minimizer = g_object_ref (UFO_METHOD (value_object));
                }

                break;
            }
        case PROP_N_LB_ITERATIONS:
            priv->n_lb_iterations = g_value_get_uint (value);
            break;

        case PROP_SHRINKAGE_PSI:
            priv->shrinkage_psi = g_value_get_float (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_linearized_bregman_method_get_property (GObject    *object,
                                                 guint      property_id,
                                                 GValue     *value,
                                                 GParamSpec *pspec)
{
    UfoIrLinearizedBregmanMethodPrivate *priv =
        UFO_IR_LINEARIZED_BREGMAN_METHOD_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_DF_MINIMIZER:
            g_value_set_object (value, priv->df_minimizer);
            break;
        case PROP_N_LB_ITERATIONS:
            g_value_set_uint (value, priv->n_lb_iterations);
            break;
        case PROP_SHRINKAGE_PSI:
            g_value_set_float (value, priv->shrinkage_psi);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_linearized_bregman_method_dispose (GObject *object)
{
    UfoIrLinearizedBregmanMethodPrivate *priv =
        UFO_IR_LINEARIZED_BREGMAN_METHOD_GET_PRIVATE (object);

    if (priv->df_minimizer) {
        g_object_unref (priv->df_minimizer);
        priv->df_minimizer = NULL;
    }

    if (priv->shrinkage) {
        g_object_unref (priv->shrinkage);
        priv->shrinkage = NULL;
    }

    G_OBJECT_CLASS (ufo_ir_linearized_bregman_method_parent_class)->dispose (object);
}

static void
ufo_ir_linearized_bregman_method_setup_real (UfoProcessor *processor,
                                             UfoResources *resources,
                                             GError       **error)
{
    UFO_PROCESSOR_CLASS (ufo_ir_linearized_bregman_method_parent_class)->setup (processor,
                                                                             resources,
                                                                             error);
    if (error && *error)
      return;

    UfoIrLinearizedBregmanMethodPrivate *priv =
        UFO_IR_LINEARIZED_BREGMAN_METHOD_GET_PRIVATE (processor);

    const gchar *filename = "ufo-ir-linearized-bregman-method.cl";
    priv->gradient_kernel = ufo_resources_get_kernel (resources, filename, "gradient", error);
    if (*error) return;
    priv->Ix_kernel = ufo_resources_get_kernel (resources, filename, "computeIx", error);
    if (*error) return;

    if (priv->df_minimizer == NULL) {
        g_set_error (error, UFO_IR_LINEARIZED_BREGMAN_METHOD_ERROR,
                     UFO_IR_LINEARIZED_BREGMAN_METHOD_ERROR_SETUP,
                     "Data-fidelity minimizer does not set.");
        return;
    }

    UfoIrProjector *projector = NULL;
    UfoProfiler    *profiler = NULL;
    gpointer        cmd_queue = NULL;
    g_object_get (processor,
                  "projection-model", &projector,
                  "command-queue", &cmd_queue,
                  "ufo-profiler", &profiler,
                  NULL);

    g_object_set (priv->df_minimizer,
                  "projection-model", projector,
                  "command-queue", cmd_queue,
                  NULL);

    g_object_set (priv->shrinkage,
                  "ufo-profiler", profiler,
                  "command-queue", cmd_queue,
                  NULL);

    ufo_processor_setup (UFO_PROCESSOR (priv->df_minimizer), resources, error);
    ufo_processor_setup (UFO_PROCESSOR (priv->shrinkage), resources, error);
}

static void compute_Ix (UfoMethod *method,
                        UfoBuffer *input,
                        UfoBuffer *output)
{
  UfoIrLinearizedBregmanMethodPrivate *priv =
        UFO_IR_LINEARIZED_BREGMAN_METHOD_GET_PRIVATE (method);

  UfoProfiler *profiler = NULL;
  gpointer    cmd_queue = NULL;
  g_object_get (method,
                "ufo-profiler", &profiler,
                "command-queue", &cmd_queue,
                NULL);

  cl_mem d_input = ufo_buffer_get_device_image (input, cmd_queue);
  cl_mem d_output = ufo_buffer_get_device_image (output, cmd_queue);

  gpointer kernel = priv->Ix_kernel;
  UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof (cl_mem), &d_input));
  UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof (cl_mem), &d_output));

  UfoRequisition requisitions;
  ufo_buffer_get_requisition (input, &requisitions);
  ufo_profiler_call (profiler, cmd_queue, kernel,
                     requisitions.n_dims,
                     requisitions.dims, NULL);
}


static void compute_grad (UfoMethod *method,
                          UfoBuffer *input,
                          UfoBuffer *output)
{
  UfoIrLinearizedBregmanMethodPrivate *priv =
        UFO_IR_LINEARIZED_BREGMAN_METHOD_GET_PRIVATE (method);

  UfoProfiler *profiler = NULL;
  gpointer    cmd_queue = NULL;
  g_object_get (method,
                "ufo-profiler", &profiler,
                "command-queue", &cmd_queue,
                NULL);

  cl_mem d_input = ufo_buffer_get_device_image (input, cmd_queue);
  cl_mem d_output = ufo_buffer_get_device_image (output, cmd_queue);

  gpointer kernel = priv->gradient_kernel;
  UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof (cl_mem), &d_input));
  UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof (cl_mem), &d_output));

  UfoRequisition requisitions;
  ufo_buffer_get_requisition (input, &requisitions);
  ufo_profiler_call (profiler, cmd_queue, kernel,
                     requisitions.n_dims,
                     requisitions.dims, NULL);
}

static gfloat l2pow2 (UfoMethod *method,
                      UfoBuffer *input)
{
  UfoResources  *resources = NULL;
  UfoProfiler   *profiler = NULL;
  gpointer       cmd_queue = NULL;
  g_object_get (method,
                "ufo-profiler",  &profiler,
                "command-queue", &cmd_queue,
                "ufo-resources", &resources,
                NULL);

  UfoBuffer *tmp = ufo_buffer_dup (input);
  ufo_op_mul (input, input, tmp, resources, cmd_queue);
  return ufo_op_l1_norm (tmp, resources, cmd_queue);
}

static gboolean
ufo_ir_linearized_bregman_method_process_real (UfoMethod *method,
                                               UfoBuffer *input,
                                               UfoBuffer *output,
                                               gpointer  pevent)
{
  //
  // Article: http://arxiv.org/pdf/1403.7543.pdf
  // Issues:
  //   - in LB-loop we do copying v to u, but it leads to changing the image
  //     by it's gradient. So I added an additional condition in the inner loop
  //   - t does not computed. It requires computing largest SVD for [bw_v bwt_q]^t
  //
  UfoIrLinearizedBregmanMethodPrivate *priv =
        UFO_IR_LINEARIZED_BREGMAN_METHOD_GET_PRIVATE (method);

  UfoResources  *resources = NULL;
  gpointer       cmd_queue = NULL;
  guint          max_iterations = 0;
  g_object_get (method,
                "command-queue", &cmd_queue,
                "ufo-resources", &resources,
                "max-iterations",   &max_iterations,
                NULL);

  g_object_set (priv->shrinkage, "shrinkage-parameter", priv->shrinkage_psi, NULL);

  UfoBuffer *u = ufo_buffer_dup (output);
  ufo_op_set (u, 0, resources, cmd_queue);

  UfoBuffer *v   = ufo_buffer_dup (output);
  UfoBuffer *q   = ufo_buffer_dup (output);

  UfoBuffer *w   = ufo_buffer_dup (output);
  UfoBuffer *p   = ufo_buffer_dup (output);
  UfoBuffer *btw_v   = ufo_buffer_dup (output);
  UfoBuffer *btw_q   = ufo_buffer_dup (output);
  gfloat    t = 0.001f;

  guint iteration = 0;
  while (iteration < 100) {
      // compute u
      ufo_method_process (UFO_METHOD (priv->df_minimizer), input, u, NULL);

      ufo_op_set (v, 0, resources, cmd_queue);
      //ufo_buffer_copy (u, v);
      ufo_op_set (q, 0, resources, cmd_queue);
      guint lb_iteration = 0;
      while (lb_iteration < 10 && iteration < 100-1) {
          compute_grad (method, u, w);
          if (lb_iteration > 0) {
            // p = 0 at iteration = 0
            // w = [grad   -I] [u    p]^t ==> w = grad(u) - Ip
            compute_Ix (method, p, p);
            ufo_op_deduction (w, p, w, resources, cmd_queue);
          }

          // compute transp(B) * w = transp([grad   -I]) * w = transp([grad(w)  -Iw])
          compute_grad (method, w, btw_v);
          compute_Ix   (method, w, btw_q);

          // compute t
          //t = l2pow2(method, w);
          //t = t / l2pow2 (method, largest SVD of matrix [grad(w) -I(w)]);

          // v_k+1 = v_k - tk * btw_v
          ufo_op_deduction2 (v, btw_v, t, v, resources, cmd_queue);

          // q_k+1 = q_k - (tk * -1 * btw_q) = q_k + tk * btw_q
          ufo_op_add2 (q, btw_q, t, q, resources, cmd_queue);

          // copy v to u
          ufo_buffer_copy (v, u);

          // do shrinkage
          ufo_method_process (priv->shrinkage, q, p, NULL);

          lb_iteration ++;
      }

      iteration++;
  }
  ufo_buffer_copy (u, output);

  return TRUE;
}

static void
ufo_method_interface_init (UfoMethodIface *iface)
{
    iface->process = ufo_ir_linearized_bregman_method_process_real;
}

static UfoCopyable *
ufo_ir_linearized_bregman_method_copy_real (gpointer origin,
                                            gpointer _copy)
{
    UfoCopyable *copy;
    if (_copy)
        copy = UFO_COPYABLE(_copy);
    else
        copy = UFO_COPYABLE (ufo_ir_linearized_bregman_method_new());

    UfoIrLinearizedBregmanMethodPrivate *priv =
        UFO_IR_LINEARIZED_BREGMAN_METHOD_GET_PRIVATE (origin);

    //
    // copy the method that minimizes data fidelity term
    gpointer df_minimizer_copy = ufo_copyable_copy (priv->df_minimizer, NULL);
    if (df_minimizer_copy) {
      g_object_set (G_OBJECT(copy), "df-minimizer", df_minimizer_copy, NULL);
    } else {
      g_error ("Error in copying Linearized Bregman method: df-minimizer was not copied.");
    }

    return copy;
}

static void
ufo_copyable_interface_init (UfoCopyableIface *iface)
{
    iface->copy = ufo_ir_linearized_bregman_method_copy_real;
}

static void
ufo_ir_linearized_bregman_method_class_init (UfoIrLinearizedBregmanMethodClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = ufo_ir_linearized_bregman_method_dispose;
    gobject_class->set_property = ufo_ir_linearized_bregman_method_set_property;
    gobject_class->get_property = ufo_ir_linearized_bregman_method_get_property;

    properties[PROP_DF_MINIMIZER] =
        g_param_spec_object("df-minimizer",
                            "Pointer to the instance of UfoIrMethod.",
                            "Pointer to the instance of UfoIrMethod",
                            UFO_TYPE_METHOD,
                            G_PARAM_READWRITE);

    properties[PROP_SHRINKAGE_PSI] =
        g_param_spec_float("shrinkage-psi",
                           "shrinkage-psi",
                           "shrinkage-psi",
                           0.0f, G_MAXFLOAT, 0.5f,
                           G_PARAM_READWRITE);

    properties[PROP_N_LB_ITERATIONS] =
        g_param_spec_uint("n-lb-iterations",
                          "n-lb-iterations",
                          "n-lb-iterations",
                          0, 1000, 10,
                          G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof(UfoIrLinearizedBregmanMethodPrivate));

    UFO_PROCESSOR_CLASS (klass)->setup = ufo_ir_linearized_bregman_method_setup_real;
}