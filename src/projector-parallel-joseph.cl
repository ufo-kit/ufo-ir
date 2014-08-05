const sampler_t r_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;
const sampler_t bp_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
const sampler_t nb_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

#define UFO_BUFFER_MAX_NDIMS 3

typedef struct {
    float detector_scale;
    float detector_offset;
} UfoParallelGeometryMeta;

typedef struct {
    gsize origin[UFO_BUFFER_MAX_NDIMS];
    gsize size[UFO_BUFFER_MAX_NDIMS];
} UfoRegion;

typedef enum {
    Vertical = 1,
    Horizontal = 0
} Direction;

typedef struct {
  guint offset;
  guint n;
  Direction direction;
} UfoProjectionsSubset;

__kernel
void FP_hor(__const         UfoParallelGeometryMeta geometry,
            __constant      float                   *sin_val,
            __constant      float                   *cos_val,
            __const         UfoProjectionsSubset    part,
            __read_only     image2d_t               volume,
            __const         UfoRegion               region,
            __read_only     image2d_t               r_sinogram,
            __write_only    image2d_t               w_sinogram,
            __const         float                   output_scale)
{
    int2 sino_coord;
    sino_coord.y = part.offset + get_global_id(1);
    sino_coord.x = get_global_id(0);

    __const float fDetStep   = -1.0f / sin_val[sino_coord.y];
            float fSliceStep = cos_val[sino_coord.y] / sin_val[sino_coord.y];

    __const float fDistCorr  = (sin_val[sino_coord.y] > 0.0f ? -fDetStep : fDetStep) * output_scale;

    float4 detected_value = 0;

    float2 volume_coord;
    volume_coord.x = 0.5f;
    volume_coord.y = (sino_coord.x + 0.5f - 0.5f * geometry.n_dets) * fDetStep +
                     (0.5f - 0.5f * geometry.vol_width) * fSliceStep + 0.5f * geometry.vol_height;

    for (int i = 0; i < geometry.vol_height; ++i) {
        detected_value += read_imagef(volume, r_sampler, volume_coord);
        volume_coord.y += fSliceStep;
        volume_coord.x += 1.0f;
    }

    float4 det_value = read_imagef(r_sinogram, nb_sampler, sino_coord);
    write_imagef (w_sinogram, sino_coord, det_value + detected_value * fDistCorr);
}

__kernel
void FP_vert(__const          UfoIrGeometry       geometry,
             __const          UfoIrProjAccessPart part,
             __constant       float               *sin_val,
             __constant       float               *cos_val,
             __read_only      image2d_t           volume,
             __read_only      image2d_t           r_sinogram,
             __write_only     image2d_t           w_sinogram,
             __const          float               output_scale)
{
    int2 sino_coord;
    sino_coord.y = part.offset + get_global_id(1);
    sino_coord.x = get_global_id(0);

    __const float fDetStep   = 1.0f / cos_val[sino_coord.y];
            float fSliceStep = sin_val[sino_coord.y] / cos_val[sino_coord.y];

    __const float fDistCorr  = (cos_val[sino_coord.y] < 0.0f ? -fDetStep : fDetStep) * output_scale;

    float4 detected_value = 0;

    float2 volume_coord;
    volume_coord.y = 0.5f;
    volume_coord.x = (sino_coord.x + 0.5f - 0.5f * geometry.n_dets) * fDetStep + // shift on X because of detector
                     (0.5f - 0.5f * geometry.vol_height) * fSliceStep +          // shift on X because of Y
                     0.5f * geometry.vol_width;                                  //

    for (int i = 0; i < geometry.vol_height; ++i) {
        detected_value += read_imagef(volume, r_sampler, volume_coord);
        volume_coord.x += fSliceStep;
        volume_coord.y += 1.0f;
    }

    float4 det_value = read_imagef(r_sinogram, nb_sampler, sino_coord);
    write_imagef (w_sinogram, sino_coord, det_value + detected_value * fDistCorr);
}

__kernel
void BP(__const     UfoIrGeometry       geometry,
        __const     float               relax_param,
        __const     UfoIrProjAccessPart part,
        __constant  float               *sin_val,
        __constant  float               *cos_val,
        __read_only  image2d_t           r_volume,
        __write_only image2d_t           w_volume,
        __read_only  image2d_t           sinogram)
{
    __const int2 vol_coord;
    vol_coord.x = get_global_id(0);
    vol_coord.y = get_global_id(1);

    __const float fX = convert_float(vol_coord.x) + 0.5f - 0.5f * geometry.vol_width;
    __const float fY = convert_float(vol_coord.y) + 0.5f - 0.5f * geometry.vol_height;

    float4 value = read_imagef(r_volume, nb_sampler, vol_coord);

    float2 sino_coord;
    sino_coord.y = part.offset;
    __const float f_coord_shift = 0.5f * geometry.n_dets;

    for (int i = 0; i < part.length; ++i) {
        float sin_theta = sin_val[i + part.offset];
        float cos_theta = cos_val[i + part.offset];

        sino_coord.x = f_coord_shift + fX * cos_theta - fY * sin_theta;
        value += read_imagef(sinogram, bp_sampler, sino_coord);
        sino_coord.y += 1.0f;
    }
    value *= relax_param;
    write_imagef (w_volume, vol_coord, value);
}


__kernel
void BPw(__const     UfoIrGeometry       geometry,
         __const     float               relax_param,
         __const     UfoIrProjAccessPart part,
         __constant  float               *sin_val,
         __constant  float               *cos_val,
         __read_only  image2d_t           r_volume,
         __write_only image2d_t           w_volume,
         __read_only  image2d_t           sinogram,
         __read_only  image2d_t           pixel_weights)
{
    __const int2 vol_coord;
    vol_coord.x = get_global_id(0);
    vol_coord.y = get_global_id(1);

    __const float fX = convert_float(vol_coord.x) + 0.5f - 0.5f * geometry.vol_width;
    __const float fY = convert_float(vol_coord.y) + 0.5f - 0.5f * geometry.vol_height;

    float4 value = 0;
    float2 sino_coord_r;
    sino_coord_r.y = part.offset;
    __const float f_coord_shift = 0.5f * geometry.n_dets;

    for (int i = 0; i < part.length; ++i) {
        float sin_theta = sin_val[i + part.offset];
        float cos_theta = cos_val[i + part.offset];
        sino_coord_r.x = f_coord_shift + fX * cos_theta - fY * sin_theta;

        value += read_imagef(sinogram, bp_sampler, sino_coord_r);
        sino_coord_r.y += 1.0f;
    }
    value = read_imagef(r_volume, nb_sampler, vol_coord) + relax_param * value / read_imagef (pixel_weights, nb_sampler, vol_coord);
    write_imagef (w_volume, vol_coord, value);
}
