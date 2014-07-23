#include <ufo-projector.h>
#include <ufo-operator-iface.h>
#include <ufo-angles.h>

static void ufo_operator_interface_init (UfoOperatorIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoProjector, ufo_projector, UFO_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_OPERATOR,
                                                ufo_operator_interface_init))

#define UFO_PROJECTOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_PROJECTOR, UfoProjectorPrivate))
gboolean projector_type_error (UfoProjector *self, GError **error);

GQuark
ufo_projector_error_quark (void)
{
    return g_quark_from_static_string ("ufo-projector-error-quark");
}

struct _UfoProjectorPrivate {
    UfoGeometry *geometry;
    UfoAngles   *angles;
};

enum {
    PROP_0,
    PROP_GEOMETRY,
    PROP_ANGLES,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoProjector *
ufo_projector_new ()
{
    return (UfoProjector *) g_object_new (UFO_TYPE_PROJECTOR,
                                            NULL);
}

static void
ufo_projector_init (UfoProjector *self)
{
    UfoProjectorPrivate *priv = NULL;
    self->priv = priv = UFO_PROJECTOR_GET_PRIVATE (self);
    priv->angles = NULL;
}

static void
ufo_projector_set_property (GObject *object,
                               guint property_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
    UfoProjectorPrivate *priv = UFO_PROJECTOR_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_GEOMETRY:
            priv->geometry = g_value_get_pointer(value);
            break;
        case PROP_ANGLES: {
            if (priv->angles){
                g_object_unref(priv->angles);
            }
            priv->angles = g_object_ref (g_value_get_pointer(value));
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_projector_get_property (GObject *object,
                               guint property_id,
                               GValue *value,
                               GParamSpec *pspec)
{
    UfoProjectorPrivate *priv = UFO_PROJECTOR_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_GEOMETRY:
            g_value_set_pointer (value, priv->geometry);
            break;
        case PROP_ANGLES:
            g_value_set_pointer (value, priv->angles);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_projector_dispose (GObject *object)
{
    UfoProjectorPrivate *priv = UFO_PROJECTOR_GET_PRIVATE (object);
    g_object_unref(priv->angles);

    G_OBJECT_CLASS (ufo_projector_parent_class)->dispose (object);
}

static void
ufo_projector_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_projector_parent_class)->finalize (object);
}

static void
_ufo_projector_FP (UfoProjector       *projector,
                      UfoBuffer            *volume,
                      UfoBuffer            *sinogram,
                      UfoProjAccessPart  *part,
                      gfloat               output_scale,
                      cl_event             *finish_event)
{
    g_error ("Method FP is not implemented.");
}

static void
_ufo_projector_BP (UfoProjector       *projector,
                      UfoBuffer            *volume,
                      UfoBuffer            *sinogram,
                      gfloat               relaxation_param,
                      UfoProjAccessPart  *part,
                      cl_event             *finish_event)
{
    g_error ("Method BP is not implemented.");
}

static void
_ufo_projector_BPw (UfoProjector      *projector,
                       UfoBuffer           *volume,
                       UfoBuffer           *sinogram,
                       UfoBuffer           *pixel_weights,
                       gfloat               relaxation_param,
                       UfoProjAccessPart *part,
                       cl_event            *finish_event)
{
    g_error ("Method BPw is not implemented.");
}


static void
_ufo_projector_setup (UfoProjector *projector,
                         UfoResources   *resources,
                         GError        **error)
{
  g_print("\n @@_ufo_projector_setup \n");

    UfoProjectorPrivate *priv = UFO_PROJECTOR_GET_PRIVATE (projector);
    if (!priv->angles) {
        g_set_error (error, UFO_PROJECTOR_ERROR, UFO_PROJECTOR_ERROR_SETUP,
                     "Property ::angles is not specified");
    }
}

static void
_ufo_projector_run (UfoOperator          *oper,
                    GValue               *input,
                    GValue               *result,
                    gpointer              cmd_queue,
                    gpointer              pfin_event)
{
  g_print ("\n _ufo_projector_run");
}

static void
_ufo_projector_grad_run (UfoOperator             *oper,
                         UfoTerm               *term,
                         GHashTable              *operands_map,
                         UfoOperationProvider    *provider,
                         GValue                  *result,
                         gpointer                 cmd_queue,
                         gpointer                 pfin_event)
{
  g_print ("\n _ufo_projector_grad_run");
}

static void
ufo_operator_interface_init (UfoOperatorIface *iface)
{
    iface->run      = _ufo_projector_run;
    iface->grad_run = _ufo_projector_grad_run;
}

static void
ufo_projector_class_init (UfoProjectorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = ufo_projector_finalize;
    gobject_class->dispose = ufo_projector_dispose;
    gobject_class->set_property = ufo_projector_set_property;
    gobject_class->get_property = ufo_projector_get_property;

    properties[PROP_GEOMETRY] =
        g_param_spec_pointer("geometry",
                             "Pointer to the instance of UfoGeometry structure",
                             "Pointer to the instance of UfoGeometry structure",
                             G_PARAM_READWRITE);

    properties[PROP_ANGLES] =
        g_param_spec_pointer("angles",
                             "Pointer to the instance of UfoAngles class",
                             "Pointer to the instance of UfoAngles class",
                             G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);


    g_type_class_add_private (gobject_class, sizeof(UfoProjectorPrivate));

    klass->FP = _ufo_projector_FP;
    klass->BP = _ufo_projector_BP;
    klass->BPw = _ufo_projector_BPw;
    klass->setup = _ufo_projector_setup;
}

void
ufo_projector_FP (UfoProjector       *projector,
                     UfoBuffer            *volume,
                     UfoBuffer            *sinogram,
                     UfoProjAccessPart  *part,
                     gfloat               output_scale,
                     cl_event             *finish_event)
{
    g_return_if_fail(projector && volume && sinogram);

    UfoProjectorClass *klass = UFO_PROJECTOR_GET_CLASS (projector);
    klass->FP(projector, volume, sinogram, part, output_scale, finish_event);
}

void
ufo_projector_BP (UfoProjector       *projector,
                     UfoBuffer            *volume,
                     UfoBuffer            *sinogram,
                     gfloat               relaxation_param,
                     UfoProjAccessPart  *part,
                     cl_event             *finish_event)
{
    g_return_if_fail(projector && volume && sinogram);

    UfoProjectorClass *klass = UFO_PROJECTOR_GET_CLASS (projector);
    klass->BP(projector, volume, sinogram, relaxation_param, part, finish_event);
}

void
ufo_projector_BPw (UfoProjector      *projector,
                      UfoBuffer           *volume,
                      UfoBuffer           *sinogram,
                      UfoBuffer           *pixel_weights,
                      gfloat              relaxation_param,
                      UfoProjAccessPart *part,
                      cl_event            *finish_event)
{
    g_return_if_fail (UFO_IS_PROJECTOR (projector) &&
                      UFO_IS_BUFFER (volume) &&
                      UFO_IS_BUFFER (sinogram) &&
                      UFO_IS_BUFFER (pixel_weights) &&
                      part);

    UfoProjectorClass *klass = UFO_PROJECTOR_GET_CLASS (projector);
    klass->BPw(projector, volume, sinogram, pixel_weights, relaxation_param, part, finish_event);
}

void
ufo_projector_setup (UfoProjector *projector,
                        UfoResources   *resources,
                        GError        **error)
{
    if (projector == NULL && resources == NULL) {
        g_set_error (error, UFO_PROJECTOR_ERROR, UFO_PROJECTOR_ERROR_SETUP,
                     "Neither argument @projector nor argument @resources are specified");
        return;
    } else if (projector == NULL) {
        g_set_error (error, UFO_PROJECTOR_ERROR, UFO_PROJECTOR_ERROR_SETUP,
                     "Argument @projector is not specified");
        return;
    } else if (resources == NULL) {
        g_set_error (error, UFO_PROJECTOR_ERROR, UFO_PROJECTOR_ERROR_SETUP,
                     "Argument @resources is not specified");
        return;
    }

    UfoProjectorClass *klass = UFO_PROJECTOR_GET_CLASS (projector);
    klass->setup(projector, resources, error);
}
