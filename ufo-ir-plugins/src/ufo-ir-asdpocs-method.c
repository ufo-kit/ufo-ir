#include "ufo-ir-asdpocs-method.h"
#include <ufo/ufo.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

static void ufo_method_interface_init (UfoMethodIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoIrAsdPocsMethod, ufo_ir_asdpocs_method, UFO_IR_TYPE_METHOD,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_METHOD,
                                                ufo_method_interface_init))

#define UFO_IR_ASDPOCS_METHOD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_ASDPOCS_METHOD, UfoIrAsdPocsMethodPrivate))

GQuark
ufo_ir_asdpocs_method_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-asdpocs-method-error-quark");
}

struct _UfoIrAsdPocsMethodPrivate {
    UfoIrMethod   *df_minimizer;
    UfoIrSparsity *sparsity;

    gfloat beta;
    gfloat beta_red;
    guint  ng;
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
                     UfoIrPriorKnowledge *prior)
{
    UfoIrAsdPocsMethodPrivate *priv = UFO_IR_ASDPOCS_METHOD_GET_PRIVATE (object);
    UfoIrSparsity *s = ufo_ir_prior_knowledge_pointer (prior, "image-sparsity");
    if (s) {
      g_clear_object(&priv->sparsity);
      priv->sparsity = g_object_ref(s);
    } else {
      g_error ("%s : received prior knowledge without \"image-sparsity\".",
               G_OBJECT_TYPE_NAME (object));
    }
}

UfoIrMethod *
ufo_ir_asdpocs_method_new (void)
{
    return UFO_IR_METHOD (g_object_new (UFO_IR_TYPE_ASDPOCS_METHOD, NULL));
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
        case IR_METHOD_PROP_PRIOR_KNOWLEDGE:
            set_prior_knowledge (object, g_value_get_pointer(value));
            break;
        case PROP_DF_MINIMIZER:
            {
                value_object = g_value_get_object (value);

                if (priv->df_minimizer)
                    g_object_unref (priv->df_minimizer);

                if (value_object != NULL) {
                    priv->df_minimizer = g_object_ref (UFO_IR_METHOD (value_object));
                }
            }
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

    if (priv->sparsity == NULL) {
        g_set_error (error, UFO_IR_ASDPOCS_METHOD_ERROR,
                     UFO_IR_ASDPOCS_METHOD_ERROR_SETUP,
                     "Sparsity does not set.");
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
    g_object_set (priv->sparsity,
                  "ufo-profiler", profiler,
                  "command-queue", cmd_queue,
                  NULL);

    ufo_processor_setup (UFO_PROCESSOR (priv->df_minimizer), resources, error);
    ufo_processor_setup (UFO_PROCESSOR (priv->sparsity), resources, error);
}

static gboolean
ufo_ir_asdpocs_method_process_real (UfoMethod *method,
                                    UfoBuffer *input,
                                    UfoBuffer *output,
                                    gpointer  pevent)
{
    UfoIrAsdPocsMethodPrivate *priv = UFO_IR_ASDPOCS_METHOD_GET_PRIVATE (method);
    UfoBuffer *x_residual = ufo_buffer_dup (output);
    UfoBuffer *b_residual = ufo_buffer_dup (input);
    UfoBuffer *x = ufo_buffer_dup (output);
    UfoBuffer *x_prev = ufo_buffer_dup (output);

    UfoResources     *resources = NULL;
    UfoIrProjector   *projector = NULL;
    cl_command_queue cmd_queue = NULL;
    guint max_iterations = 0;
    g_object_get (method,
                  "projection-model", &projector,
                  "command-queue",    &cmd_queue,
                  "ufo-resources",    &resources,
                  "max-iterations",   &max_iterations,
                  NULL);

    UfoRequisition b_req;
    ufo_buffer_get_requisition (input, &b_req);
    UfoIrProjectionsSubset complete_set;
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
        ufo_method_process (UFO_METHOD (priv->df_minimizer), input, x, NULL);
	ufo_op_POSC (x, x, resources, cmd_queue);
	ufo_buffer_copy (x, output);
        ufo_buffer_copy (input, b_residual);

        ufo_ir_projector_FP (projector, x, b_residual, &complete_set, -1, NULL);
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
        ufo_ir_sparsity_minimize (priv->sparsity, x, x, NULL);

        ufo_op_deduction (x, x_prev, x_residual, resources, cmd_queue);
        dg = ufo_op_l1_norm (x_residual, resources, cmd_queue);
	g_print ("\nx - x_prev l1: %f", dg);

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
    iface->process = ufo_ir_asdpocs_method_process_real;
}

static void
ufo_ir_asdpocs_method_class_init (UfoIrAsdPocsMethodClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = ufo_ir_asdpocs_method_dispose;
    gobject_class->set_property = ufo_ir_asdpocs_method_set_property;
    gobject_class->get_property = ufo_ir_asdpocs_method_get_property;

    g_object_class_override_property(gobject_class,
                                     IR_METHOD_PROP_PRIOR_KNOWLEDGE,
                                     "prior-knowledge");

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

static void
ufo_ir_asdpocs_method_init (UfoIrAsdPocsMethod *self)
{
    UfoIrAsdPocsMethodPrivate *priv = NULL;
    self->priv = priv = UFO_IR_ASDPOCS_METHOD_GET_PRIVATE (self);
    priv->df_minimizer = NULL;
    priv->sparsity = NULL;
    priv->beta = 1.0f;
    priv->beta_red = 0.995f;
    priv->ng = 20;
    priv->alpha = 0.2f;
    priv->alpha_red = 0.95f;
    priv->r_max = 0.95f;
}
