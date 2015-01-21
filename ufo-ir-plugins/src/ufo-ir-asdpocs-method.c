#include "ufo-ir-asdpocs-method.h"
#include <ufo/ufo.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

static void ufo_method_interface_init (UfoMethodIface *iface);
static void ufo_copyable_interface_init (UfoCopyableIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrAsdPocsMethod, ufo_ir_asdpocs_method, UFO_IR_TYPE_METHOD,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_METHOD,
                                                ufo_method_interface_init)
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_COPYABLE,
                                                ufo_copyable_interface_init))

#define UFO_IR_ASDPOCS_METHOD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_ASDPOCS_METHOD, UfoIrAsdPocsMethodPrivate))

GQuark
ufo_ir_asdpocs_method_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-asdpocs-method-error-quark");
}

struct _UfoIrAsdPocsMethodPrivate {
    UfoIrMethod   *df_minimizer;
    UfoMethod     *tv_stdesc;

    gfloat beta;
    gfloat beta_red;
    guint  ng;
    gfloat alpha;
    gfloat alpha_red;
    gfloat r_max;
};

enum {
    PROP_0 = 0,
    PROP_DF_MINIMIZER,
    PROP_BETA,
    PROP_BETA_RED,
    PROP_NG,
    PROP_ALPHA,
    PROP_R_MAX,
    PROP_ALPHA_RED,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

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
ufo_ir_asdpocs_method_new (void)
{
    return UFO_IR_METHOD (g_object_new (UFO_IR_TYPE_ASDPOCS_METHOD, NULL));
}

static void
ufo_ir_asdpocs_method_init (UfoIrAsdPocsMethod *self)
{
    UfoIrAsdPocsMethodPrivate *priv = NULL;
    self->priv = priv = UFO_IR_ASDPOCS_METHOD_GET_PRIVATE (self);
    priv->df_minimizer = NULL;
    priv->tv_stdesc = NULL;
    priv->beta = 1.0f;
    priv->beta_red = 0.995f;
    priv->ng = 20;
    priv->alpha = 0.2f;
    priv->alpha_red = 0.95f;
    priv->r_max = 0.95f;
}


static void
ufo_ir_asdpocs_method_set_property (GObject      *object,
                                    guint        property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
    UfoIrAsdPocsMethodPrivate *priv = UFO_IR_ASDPOCS_METHOD_GET_PRIVATE (object);

    GObject *value_object;

    switch (property_id) {
        case PROP_DF_MINIMIZER:
            {
                value_object = g_value_get_object (value);

                if (priv->df_minimizer) {
                    g_object_unref (priv->df_minimizer);
                }
                if (value_object != NULL) {
                    priv->df_minimizer = g_object_ref (UFO_IR_METHOD (value_object));
                }

                break;
            }
        case PROP_BETA:
            priv->beta = g_value_get_float (value);
            break;
        case PROP_BETA_RED:
            priv->beta_red = g_value_get_float (value);
            break;
        case PROP_NG:
            priv->ng = g_value_get_uint (value);
            break;
        case PROP_ALPHA:
            priv->alpha = g_value_get_float (value);
            break;
        case PROP_ALPHA_RED:
            priv->alpha_red = g_value_get_float (value);
            break;
        case PROP_R_MAX:
            priv->r_max = g_value_get_float (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_asdpocs_method_get_property (GObject    *object,
                                    guint      property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
    UfoIrAsdPocsMethodPrivate *priv = UFO_IR_ASDPOCS_METHOD_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_DF_MINIMIZER:
            g_value_set_object (value, priv->df_minimizer);
            break;
        case PROP_BETA:
            g_value_set_float (value, priv->beta);
            break;
        case PROP_BETA_RED:
            g_value_set_float (value, priv->beta_red);
            break;
        case PROP_NG:
            g_value_set_uint (value, priv->ng);
            break;
        case PROP_ALPHA:
            g_value_set_float (value, priv->alpha);
            break;
        case PROP_ALPHA_RED:
            g_value_set_float (value, priv->alpha_red);
            break;
        case PROP_R_MAX:
            g_value_set_float (value, priv->r_max);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_ir_asdpocs_method_dispose (GObject *object)
{
    UfoIrAsdPocsMethodPrivate *priv = UFO_IR_ASDPOCS_METHOD_GET_PRIVATE (object);

    if (priv->df_minimizer) {
        g_object_unref (priv->df_minimizer);
        priv->df_minimizer = NULL;
    }

    G_OBJECT_CLASS (ufo_ir_asdpocs_method_parent_class)->dispose (object);
}

static void
ufo_ir_asdpocs_method_setup_real (UfoProcessor *processor,
                                  UfoResources *resources,
                                  GError       **error)
{
    UFO_PROCESSOR_CLASS (ufo_ir_asdpocs_method_parent_class)->setup (processor,
                                                                     resources,
                                                                     error);
    if (error && *error)
      return;

    UfoIrAsdPocsMethodPrivate *priv = UFO_IR_ASDPOCS_METHOD_GET_PRIVATE (processor);
    if (priv->df_minimizer == NULL) {
        g_set_error (error, UFO_IR_ASDPOCS_METHOD_ERROR,
                     UFO_IR_ASDPOCS_METHOD_ERROR_SETUP,
                     "Data-fidelity minimizer does not set.");
        return;
    }

    UfoPluginManager *plugin_manager = ufo_plugin_manager_new ();
    priv->tv_stdesc = UFO_METHOD (ufo_plugin_manager_get_plugin (plugin_manager,
                                                  "ufo_math_tvstd_method_new",
                                                  "libufo_math_tvstd_method.so",
                                                   error));

    if (priv->tv_stdesc == NULL) {
        g_set_error (error, UFO_IR_ASDPOCS_METHOD_ERROR,
                     UFO_IR_ASDPOCS_METHOD_ERROR_SETUP,
                     "TV-steepest descent methods was not loaded.");
        return;
    }

    UfoIrProjector *projector = NULL;
    UfoProfiler  *profiler = NULL;
    gpointer cmd_queue = NULL;
    g_object_get (processor,
                  "projection-model", &projector,
                  "command-queue", &cmd_queue,
                  "ufo-profiler", &profiler,
                  NULL);
    g_object_set (priv->df_minimizer,
                  "projection-model", projector,
                  "command-queue", cmd_queue,
                  NULL);

    g_object_set (priv->tv_stdesc,
                  "ufo-profiler", profiler,
                  "command-queue", cmd_queue,
                  NULL);

    ufo_processor_setup (UFO_PROCESSOR (priv->df_minimizer), resources, error);
    ufo_processor_setup (UFO_PROCESSOR (priv->tv_stdesc), resources, error);
}

static gboolean
ufo_ir_asdpocs_method_process_real (UfoMethod *method,
                                    UfoBuffer *input,
                                    UfoBuffer *output,
                                    gpointer  pevent)
{
    UfoIrAsdPocsMethodPrivate *priv = UFO_IR_ASDPOCS_METHOD_GET_PRIVATE (method);
    UfoResources     *resources = NULL;
    UfoIrProjector   *projector = NULL;
    gpointer         *cmd_queue  = NULL;
    guint             max_iterations = 0;

    gfloat beta      = 0;
    gfloat beta_red  = 0;
    gfloat ng        = 0;
    gfloat alpha     = 0;
    gfloat alpha_red = 0;
    gfloat r_max     = 0;

    g_object_get (method,
                  "projection-model", &projector,
                  "command-queue",    &cmd_queue,
                  "ufo-resources",    &resources,
                  "max-iterations",   &max_iterations,

                  // method parameters
                  "beta",             &beta,
                  "beta-red",         &beta_red,
                  "ng",               &ng,
                  "alpha",            &alpha,
                  "alpha-red",        &alpha_red,
                  "r-max",            &r_max,
                  NULL);

    UfoIrGeometry *geometry = NULL;
    g_object_get (projector, "geometry", &geometry, NULL);

    // parameters
    gfloat dp = 1.0f, dd = 1.0f, dg = 1.0f, dtgv = 1.0f;

    UfoBuffer *x      = ufo_buffer_dup (output);
    UfoBuffer *x_prev = ufo_buffer_dup (output);

    ufo_op_set (x, 0, resources, cmd_queue);
    ufo_op_set (x_prev, 0, resources, cmd_queue);

    UfoBuffer *x_residual = ufo_buffer_dup (output);
    UfoBuffer *b_residual = ufo_buffer_dup (input);

    guint n_subsets = 0;
    UfoIrProjectionsSubset *subset = generate_subsets (geometry, &n_subsets);

    guint iteration = 0;
    while (iteration < max_iterations)
    {
      //
      // run method to minimize data fidelity term
      g_object_set (priv->df_minimizer, "relaxation-factor", beta, NULL);
      ufo_method_process (UFO_METHOD (priv->df_minimizer), input, x, NULL);

      //
      // impose positive constraint: if x_i < 0 then x_i = 0
      ufo_op_POSC (x, x, resources, cmd_queue);

      //
      // save result as an result
      ufo_buffer_copy (x, output);

      //
      // Find residual between the simulated and real measurements
      ufo_buffer_copy (input, b_residual);
      for (guint i = 0 ; i < n_subsets; ++i) {
          ufo_ir_projector_FP (projector,
                               x,
                               b_residual,
                               &subset[i],
                               -1.0f, NULL);
      }

      //
      // compute L1-norm of the residual of measurements
      dd = ufo_op_l1_norm (b_residual, resources, cmd_queue);

      //
      // compute L1-norm of the residual of reconstructions
      ufo_op_deduction (x, x_prev, x_residual, resources, cmd_queue);
      dp = ufo_op_l1_norm (x_residual, resources, cmd_queue);

      //
      // compute relaxation factor for minimizing regularization term
      if (iteration == 0) {
          dtgv = priv->alpha * dp;
      }

      //
      // save current solution
      ufo_buffer_copy (x, x_prev);
      clFinish((cl_command_queue)cmd_queue);

      //
      //
      g_object_set (priv->tv_stdesc, "relaxation-factor", dtgv, NULL);
      ufo_method_process (UFO_METHOD (priv->tv_stdesc), x, x, NULL);

      //
      // compute new regularization coefficient
      const gfloat epsilon = 0.001f;
      ufo_op_deduction (x, x_prev, x_residual, resources, cmd_queue);
      dg = ufo_op_l1_norm (x_residual, resources, cmd_queue);
      priv->beta *= priv->beta_red;

      //
      // compute relaxation factor for minimizing regularization term
      if (dg > priv->r_max * dp && dd > epsilon) {
          dtgv *= priv->alpha_red;
      }

      iteration++;
    }

    return TRUE;
}

static void
ufo_method_interface_init (UfoMethodIface *iface)
{
    iface->process = ufo_ir_asdpocs_method_process_real;
}

static UfoCopyable *
ufo_ir_asdpocs_method_copy_real (gpointer origin,
                                 gpointer _copy)
{
    UfoCopyable *copy;
    if (_copy)
        copy = UFO_COPYABLE(_copy);
    else
        copy = UFO_COPYABLE (ufo_ir_asdpocs_method_new());

    UfoIrAsdPocsMethodPrivate *priv = UFO_IR_ASDPOCS_METHOD_GET_PRIVATE (origin);

    //
    // copy the method that minimizes data fidelity term
    gpointer df_minimizer_copy = ufo_copyable_copy (priv->df_minimizer, NULL);
    if (df_minimizer_copy) {
      g_object_set (G_OBJECT(copy), "df-minimizer", df_minimizer_copy, NULL);
    } else {
      g_error ("Error in copying ASD-POCS method: df-minimizer was not copied.");
    }

    //
    // copy other parameters
    g_object_set (G_OBJECT(copy),
                  "beta", priv->beta,
                  "beta-red", priv->beta_red,
                  "ng", priv->ng,
                  "alpha", priv->alpha,
                  "alpha-red", priv->alpha_red,
                  "r-max", priv->r_max,
                  NULL);

    return copy;
}

static void
ufo_copyable_interface_init (UfoCopyableIface *iface)
{
    iface->copy = ufo_ir_asdpocs_method_copy_real;
}

static void
ufo_ir_asdpocs_method_class_init (UfoIrAsdPocsMethodClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = ufo_ir_asdpocs_method_dispose;
    gobject_class->set_property = ufo_ir_asdpocs_method_set_property;
    gobject_class->get_property = ufo_ir_asdpocs_method_get_property;


    properties[PROP_DF_MINIMIZER] =
        g_param_spec_object("df-minimizer",
                            "Pointer to the instance of UfoIrMethod.",
                            "Pointer to the instance of UfoIrMethod",
                            UFO_IR_TYPE_METHOD,
                            G_PARAM_READWRITE);

    properties[PROP_BETA] =
        g_param_spec_float("beta",
                           "Beta",
                           "Beta",
                           0.0f, G_MAXFLOAT, 1.0f,
                           G_PARAM_READWRITE);

    properties[PROP_BETA_RED] =
        g_param_spec_float("beta-red",
                           "Beta red",
                           "Beta red",
                            0.0f, G_MAXFLOAT, 0.995f,
                            G_PARAM_READWRITE);

    properties[PROP_NG] =
        g_param_spec_uint("ng",
                          "NG",
                          "NG",
                          0, 1000, 20,
                          G_PARAM_READWRITE);

    properties[PROP_ALPHA] =
        g_param_spec_float("alpha",
                           "Alpha",
                           "Alpha",
                           0.0f, G_MAXFLOAT, 0.2f,
                           G_PARAM_READWRITE);

    properties[PROP_ALPHA_RED] =
        g_param_spec_float("alpha-red",
                           "Alpha Red",
                           "Alpha Red",
                           0.0f, G_MAXFLOAT, 0.95f,
                           G_PARAM_READWRITE);

    properties[PROP_R_MAX] =
        g_param_spec_float("r-max",
                           "R-max",
                           "R-max",
                           0.0f, G_MAXFLOAT, 0.95f,
                           G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof(UfoIrAsdPocsMethodPrivate));

    UFO_PROCESSOR_CLASS (klass)->setup = ufo_ir_asdpocs_method_setup_real;
}