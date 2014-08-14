#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "ufo-ir-asdpocs.h"
#include <ufo/ir/ufo-sparsity-iface.h>
#include <ufo/ir/ufo-prior-knowledge.h>
#include <ufo/ufo.h>
#include <math.h>

static void ufo_method_interface_init (UfoMethodIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrASDPOCS, ufo_ir_asdpocs, UFO_TYPE_IR_METHOD,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_METHOD,
                                                ufo_method_interface_init))

#define UFO_IR_ASDPOCS_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_IR_ASDPOCS, UfoIrASDPOCSPrivate))

GQuark
ufo_ir_asdpocs_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-asdpocs-error-quark");
}

struct _UfoIrASDPOCSPrivate {
  UfoIrMethod *df_minimizer;
  UfoSparsity *sparsity;

  gfloat beta;
  gfloat beta_red;
  guint ng;
  gfloat alpha;
  gfloat alpha_red;
  gfloat r_max;
};

enum {
    PROP_0 = N_IR_METHOD_VIRTUAL_PROPERTIES,
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

static void
set_prior_knowledge (GObject *object,
                     UfoPriorKnowledge *prior)
{
    UfoIrASDPOCSPrivate *priv = UFO_IR_ASDPOCS_GET_PRIVATE (object);
    UfoSparsity *s = ufo_prior_knowledge_pointer (prior, "image-sparsity");
    if (s) {
      priv->sparsity = g_object_ref(s);
    } else {
      g_error ("%s : received prior knowledge without \"image-sparsity\".",
               G_OBJECT_TYPE_NAME (object));
    }
}

UfoIrMethod *
ufo_ir_asdpocs_new (void)
{
    return UFO_IR_METHOD (g_object_new (UFO_TYPE_IR_ASDPOCS, NULL));
}

static void
ufo_ir_asdpocs_init (UfoIrASDPOCS *self)
{
    UfoIrASDPOCSPrivate *priv = NULL;
    self->priv = priv = UFO_IR_ASDPOCS_GET_PRIVATE (self);
    priv->df_minimizer = NULL;
    priv->sparsity = NULL;
    priv->beta = 1.0f;
    priv->beta_red = 0.995f;
    priv->ng = 20;
    priv->alpha = 0.2f;
    priv->alpha_red = 0.95f;
    priv->r_max = 0.95f;
}

static void
ufo_ir_asdpocs_set_property (GObject      *object,
                             guint        property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
    UfoIrASDPOCSPrivate *priv = UFO_IR_ASDPOCS_GET_PRIVATE (object);

    switch (property_id) {
        case IR_METHOD_PROP_PRIOR_KNOWLEDGE:
            set_prior_knowledge (object, g_value_get_pointer(value));
            break;
        case PROP_DF_MINIMIZER:
            g_clear_object(&priv->df_minimizer);
            priv->df_minimizer = g_object_ref (g_value_get_pointer(value));
            break;
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
ufo_ir_asdpocs_get_property (GObject    *object,
                             guint      property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
    UfoIrASDPOCSPrivate *priv = UFO_IR_ASDPOCS_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_DF_MINIMIZER:
            g_value_set_pointer (value, priv->df_minimizer);
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
ufo_ir_asdpocs_dispose (GObject *object)
{
    UfoIrASDPOCSPrivate *priv = UFO_IR_ASDPOCS_GET_PRIVATE (object);
    g_clear_object(&priv->df_minimizer);

    G_OBJECT_CLASS (ufo_ir_asdpocs_parent_class)->dispose (object);
}

static void
ufo_ir_asdpocs_setup_real (UfoProcessor *processor,
                           UfoResources *resources,
                           GError       **error)
{
    UFO_PROCESSOR_CLASS (ufo_ir_asdpocs_parent_class)->setup (processor, resources, error);
    if (error && *error)
      return;

    UfoIrASDPOCSPrivate *priv = UFO_IR_ASDPOCS_GET_PRIVATE (processor);
    if (priv->df_minimizer == NULL) {
        g_set_error (error, UFO_IR_ASDPOCS_ERROR, UFO_IR_ASDPOCS_ERROR_SETUP,
                     "Data-fidelity minimizer does not set.");
        return;
    }

    if (priv->sparsity == NULL) {
        g_set_error (error, UFO_IR_ASDPOCS_ERROR, UFO_IR_ASDPOCS_ERROR_SETUP,
                     "Sparsity does not set.");
        return;
    }

    UfoProjector *projector = NULL;
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
    g_object_set (priv->sparsity,
                  "ufo-profiler", profiler,
                  "command-queue", cmd_queue,
                  NULL);
    ufo_processor_setup (priv->df_minimizer, resources, error);
    ufo_processor_setup (priv->sparsity, resources, error);
}

static gboolean
ufo_ir_asdpocs_process_real (UfoMethod *method,
                             UfoBuffer *input,
                             UfoBuffer *output)
{
    UfoIrASDPOCSPrivate *priv = UFO_IR_ASDPOCS_GET_PRIVATE (method);
    UfoBuffer *x_residual = ufo_buffer_dup (output);
    UfoBuffer *b_residual = ufo_buffer_dup (input);
    UfoBuffer *x = ufo_buffer_dup (output);
    UfoBuffer *x_prev = ufo_buffer_dup (output);

    UfoResources *resources = NULL;
    UfoProjector *projector = NULL;
    gpointer     *cmd_queue = NULL;
    guint max_iterations = 0;
    g_object_get (method,
                  "projection-model", &projector,
                  "command-queue", &cmd_queue,
                  "ufo-resources", &resources,
                  "max-iterations", &max_iterations,
                  NULL);

    UfoRequisition b_req;
    ufo_buffer_get_requisition (input, &b_req);
    UfoProjectionsSubset complete_set;
    complete_set.offset = 0;
    complete_set.n = b_req.dims[1];
    complete_set.direction = Vertical;

    // parameters
    gfloat dp = 1.0f, dd = 1.0f, dg = 1.0f, dtgv = 1.0f;
    guint iteration = 0;

    ufo_op_set (x, 0, resources, cmd_queue);
    ufo_op_set (x_prev, 0, resources, cmd_queue);

    while (iteration < max_iterations)
    {
        clFinish(cmd_queue);
        g_object_set (priv->df_minimizer,
                      "relaxation-factor", priv->beta,
                      NULL);
        ufo_method_process (priv->df_minimizer, input, x);

        ufo_buffer_copy (x, output);
        ufo_buffer_copy (input, b_residual);

        ufo_projector_FP (projector, x, b_residual, &complete_set, -1, NULL);
        dd = ufo_op_l1_norm (b_residual, resources, cmd_queue);

        ufo_op_deduction (x, x_prev, x_residual, resources, cmd_queue);
        dp = ufo_op_l1_norm (x_residual, resources, cmd_queue);

        if (iteration == 0)
          dtgv = priv->alpha * dp;

        ufo_buffer_copy (x, x_prev);
        clFinish(cmd_queue);
        g_object_set (priv->sparsity,
                      "relaxation-factor", dtgv,
                      NULL);
        ufo_sparsity_minimize (priv->sparsity, x, x);


        ufo_op_deduction (x, x_prev, x_residual, resources, cmd_queue);
        dg = ufo_op_l1_norm (x_residual, resources, cmd_queue);

        if (dg > priv->r_max * dp && dd > 0.001f)
          dtgv *= priv->alpha_red;

        priv->beta *= priv->beta_red;
        iteration++;
    }

    return TRUE;
}

static void
ufo_method_interface_init (UfoMethodIface *iface)
{
    iface->process = ufo_ir_asdpocs_process_real;
}

static void
ufo_ir_asdpocs_class_init (UfoIrASDPOCSClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = ufo_ir_asdpocs_dispose;
    gobject_class->set_property = ufo_ir_asdpocs_set_property;
    gobject_class->get_property = ufo_ir_asdpocs_get_property;

    g_object_class_override_property(gobject_class,
                                     IR_METHOD_PROP_PRIOR_KNOWLEDGE,
                                     "prior-knowledge");

    properties[PROP_DF_MINIMIZER] =
        g_param_spec_pointer("df-minimizer",
                             "Pointer to the instance of UfoIrMethod.",
                             "Pointer to the instance of UfoIrMethod",
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

    g_type_class_add_private (gobject_class, sizeof(UfoIrASDPOCSPrivate));

    UFO_PROCESSOR_CLASS (klass)->setup = ufo_ir_asdpocs_setup_real;
}