#include "ufo-gradient-sparsity.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

static void ufo_sparsity_interface_init (UfoSparsityIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoGradientSparsity, ufo_gradient_sparsity, UFO_TYPE_PROCESSOR,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_SPARSITY,
                                                ufo_sparsity_interface_init))

#define UFO_GRADIENT_SPARSITY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_GRADIENT_SPARSITY, UfoGradientSparsityPrivate))

struct _UfoGradientSparsityPrivate {
    guint     n_iters;
    gfloat    relaxation_factor;
    UfoBuffer *grad;

    gpointer kernel;
};

enum {
    PROP_0,
    PROP_NUM_ITERS,
    PROP_RELAXATION_FACTOR,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoSparsity *
ufo_gradient_sparsity_new (void)
{
    return (UfoSparsity *) g_object_new (UFO_TYPE_GRADIENT_SPARSITY,
                                         NULL);
}

static void
ufo_gradient_sparsity_setup_real (UfoProcessor *sparsity,
                                  UfoResources *resources,
                                  GError       **error)
{
    UFO_PROCESSOR_CLASS (ufo_gradient_sparsity_parent_class)->setup (sparsity, resources, error);
    if (error && *error)
        return;

    UfoGradientSparsityPrivate *priv = UFO_GRADIENT_SPARSITY_GET_PRIVATE (sparsity);
    priv->kernel = ufo_resources_get_kernel (resources,
                                             "sparsity-gradient.cl",
                                             "l1_grad",
                                              error);
}

static void
ufo_gradient_sparsity_set_property (GObject *object,
                                    guint property_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
    UfoGradientSparsityPrivate *priv = UFO_GRADIENT_SPARSITY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_NUM_ITERS:
            priv->n_iters = g_value_get_uint (value);
            break;
        case PROP_RELAXATION_FACTOR:
            priv->relaxation_factor = g_value_get_float (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_gradient_sparsity_get_property (GObject *object,
                                    guint property_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
    UfoGradientSparsityPrivate *priv = UFO_GRADIENT_SPARSITY_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_NUM_ITERS:
            g_value_set_uint (value, priv->n_iters);
            break;
        case PROP_RELAXATION_FACTOR:
            g_value_set_float (value, priv->relaxation_factor);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_gradient_sparsity_dispose (GObject *object)
{
    //UfoGradientSparsityPrivate *priv = UFO_GRADIENT_SPARSITY_GET_PRIVATE (object);
    G_OBJECT_CLASS (ufo_gradient_sparsity_parent_class)->dispose (object);
}

static void
ufo_gradient_sparsity_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_gradient_sparsity_parent_class)->finalize (object);
}

static gboolean
ufo_gradient_sparsity_minimize_real (UfoSparsity *sparsity,
                                     UfoBuffer *input,
                                     UfoBuffer *output)
{
    UfoGradientSparsityPrivate *priv = UFO_GRADIENT_SPARSITY_GET_PRIVATE (sparsity);
    UfoResources *resources = NULL;
    UfoProfiler  *profiler = NULL;
    gpointer     *cmd_queue = NULL;
    gpointer     kernel = priv->kernel;
    g_object_get (sparsity,
                  "command-queue", &cmd_queue,
                  "ufo-resources", &resources,
                  "ufo-profiler", &profiler,
                  NULL);

    if (priv->grad) {
        UfoRequisition requisition;
        ufo_buffer_get_requisition (input, &requisition);
        ufo_buffer_resize (priv->grad, &requisition);
    }
    else {
        priv->grad = ufo_buffer_dup (input);
    }
    // is it really need?
    ufo_op_set (priv->grad, 0, resources, cmd_queue);

    UfoRequisition input_req;
    ufo_buffer_get_requisition (input, &input_req);

    cl_mem d_input = ufo_buffer_get_device_image (input, cmd_queue);
    cl_mem d_grad = ufo_buffer_get_device_image (priv->grad, cmd_queue);

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 0, sizeof(cl_mem), &d_input));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (kernel, 1, sizeof(cl_mem), &d_grad));

    gfloat factor = 0.0f, l1 = 0.0f;
    guint iteration = 0;

    while (iteration < priv->n_iters) {
        ufo_profiler_call (profiler, cmd_queue, kernel, input_req.n_dims,
                           input_req.dims, NULL);

        l1 = ufo_op_l1_norm (priv->grad, resources, cmd_queue);
        factor = priv->relaxation_factor / l1;
        ufo_op_deduction2 (input, priv->grad, factor, output, resources, cmd_queue);
        iteration++;
    }

    return FALSE;
}

static void
ufo_sparsity_interface_init (UfoSparsityIface *iface)
{
    iface->minimize = ufo_gradient_sparsity_minimize_real;
}

static void
ufo_gradient_sparsity_class_init (UfoGradientSparsityClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = ufo_gradient_sparsity_finalize;
    gobject_class->dispose = ufo_gradient_sparsity_dispose;
    gobject_class->set_property = ufo_gradient_sparsity_set_property;
    gobject_class->get_property = ufo_gradient_sparsity_get_property;

    properties[PROP_RELAXATION_FACTOR] =
        g_param_spec_float("relaxation-factor",
                           "The relaxation parameter.",
                           "The relaxation parameter.",
                           0.0f, G_MAXFLOAT, 0.5f,
                           G_PARAM_READWRITE);

    properties[PROP_NUM_ITERS] =
        g_param_spec_uint ("num-iters",
                           "Number of iterations",
                           "Number of iterations",
                           0, 1000, 20,
                           G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof (UfoGradientSparsityPrivate));

    UFO_PROCESSOR_CLASS (klass)->setup = ufo_gradient_sparsity_setup_real;
}

static void
ufo_gradient_sparsity_init (UfoGradientSparsity *self)
{
    UfoGradientSparsityPrivate *priv = NULL;
    self->priv = priv = UFO_GRADIENT_SPARSITY_GET_PRIVATE (self);
    priv->n_iters = 20;
    priv->relaxation_factor = 1.0f;
    priv->grad = NULL;
}