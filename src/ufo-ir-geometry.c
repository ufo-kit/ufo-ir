#include <ufo-ir-geometry.h>
#include <math.h>

G_DEFINE_TYPE (UfoIrGeometry, ufo_ir_geometry, G_TYPE_OBJECT)
#define UFO_IR_ANGLES_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_IR_TYPE_GEOMETRY, UfoIrGeometryPrivate))
gboolean angles_type_error (UfoIrGeometry *self, GError **error);

GQuark
ufo_ir_angles_error_quark (void)
{
    return g_quark_from_static_string ("ufo-ir-angles-error-quark");
}

struct _UfoIrGeometryProvate {
    cl_context context;
    cl_mem sin_lut;
    cl_mem cos_lut;
    gfloat *host_sin_lut;
    gfloat *host_cos_lut;
};

enum {
    PROP_0,
    PROP_NUMBER_OF_ANGLES,
    PROP_ANGLE_STEP,
    PROP_ANGLE_OFFSET,
    N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoIrGeometry *
ufo_ir_geometry_new ()
{
    return (UfoIrGeometry *) g_object_new (UFO_IR_TYPE_ANGLES,
                                           NULL);
}