const sampler_t nb_clamp_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
const sampler_t nb_clamp_edge_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE  | CLK_FILTER_NEAREST;
const sampler_t linear_clamp_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;
const sampler_t linear_clamp_edge_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

#define BLOCK_SIZE 64
#define UFO_BUFFER_MAX_NDIMS 3

typedef struct {
    long origin[UFO_BUFFER_MAX_NDIMS];
    long size[UFO_BUFFER_MAX_NDIMS];
} UfoRegion;

typedef struct {
  unsigned long height;
  unsigned long width;
  unsigned long depth;

  unsigned long n_dets;
  unsigned long n_angles;
} UfoGeometryDims;

typedef struct {
    float det_scale;
    float axis_pos;
} UfoParallelGeometrySpec;

typedef enum {
    Vertical = 1,
    Horizontal = 0
} Direction;

typedef struct {
  uint offset;
  uint n;
  Direction direction;
} UfoProjectionsSubset;

__kernel
void FP_hor(__read_only     image2d_t               volume,
            __const         UfoRegion               region,
            __read_only     image2d_t               r_sinogram,
            __write_only    image2d_t               w_sinogram,
            __const         float                   output_scale,
            __constant      float                   *sin_val,
            __constant      float                   *cos_val,
            __const         UfoGeometryDims         dimensions,
            __const         UfoParallelGeometrySpec geom_spec,
            __const         UfoProjectionsSubset    part)
{
    int2 sino_coord;
    sino_coord.y = part.offset + get_global_id(1);
    sino_coord.x = get_global_id(0);

    __const float fDetStep   = -1.0f / sin_val[sino_coord.y];
            float fSliceStep = cos_val[sino_coord.y] / sin_val[sino_coord.y];

    __const float fDistCorr  = (sin_val[sino_coord.y] > 0.0f ? -fDetStep : fDetStep) * output_scale;
    __const float f_axis_pos = geom_spec.axis_pos;

    float4 detected_value = 0.0f;

    float2 volume_coord;
    volume_coord.x = 0.5f;
    volume_coord.y = (sino_coord.x + 0.5f - 0.5f * dimensions.n_dets) * fDetStep +
                     (0.5f - f_axis_pos/*0.5f * dimensions.width*/) * fSliceStep + 0.5f * dimensions.height;

    // split up the calculation by parts to increse percision
    int    n_blocks  = ceil(convert_float(dimensions.width) / BLOCK_SIZE);
    float4 inner_sum;

    for (int j = 0; j < n_blocks; ++j) {
        inner_sum = 0.0f;
        for (int i = 0; i < BLOCK_SIZE; i++) {
            inner_sum += read_imagef(volume, linear_clamp_sampler, volume_coord);
            volume_coord.y += fSliceStep;
            volume_coord.x += 1.0f;
        }
        detected_value += inner_sum;
    }

    float4 det_value = read_imagef(r_sinogram, nb_clamp_sampler, sino_coord) + detected_value * fDistCorr;
    write_imagef (w_sinogram, sino_coord, det_value);
}

__kernel
void FP_vert(__read_only     image2d_t               volume,
             __const         UfoRegion               region,
             __read_only     image2d_t               r_sinogram,
             __write_only    image2d_t               w_sinogram,
             __const         float                   output_scale,
             __constant      float                   *sin_val,
             __constant      float                   *cos_val,
             __const         UfoGeometryDims         dimensions,
             __const         UfoParallelGeometrySpec geom_spec,
             __const         UfoProjectionsSubset    part)
{
    int2 sino_coord;
    sino_coord.y = part.offset + get_global_id(1);
    sino_coord.x = get_global_id(0);

    __const float fDetStep   = 1.0f / cos_val[sino_coord.y];
            float fSliceStep = sin_val[sino_coord.y] / cos_val[sino_coord.y];

    __const float fDistCorr  = (cos_val[sino_coord.y] < 0.0f ? -fDetStep : fDetStep) * output_scale;
    __const float f_axis_pos = geom_spec.axis_pos;
    float4 detected_value = 0.0f;

    float2 volume_coord;
    volume_coord.y = 0.5f;
    volume_coord.x = (0.5f + sino_coord.x - 0.5f * dimensions.n_dets) * fDetStep + // shift on X because of detector
                     (0.5f - f_axis_pos/*0.5f * dimensions.height*/) * fSliceStep + // shift on X because of Y
                     0.5f * dimensions.width;

    // split up the calculation by parts to increse percision
    int    n_blocks  = ceil(convert_float(dimensions.height) / BLOCK_SIZE);
    float4 inner_sum;

    for (int j = 0; j < n_blocks; ++j) {
        inner_sum = 0.0f;
        for (int i = 0; i < BLOCK_SIZE; i++) {
            inner_sum += read_imagef(volume, linear_clamp_sampler, volume_coord);
            volume_coord.x += fSliceStep;
            volume_coord.y += 1.0f;
        }
        detected_value += inner_sum;
    }

    float4 det_value = read_imagef(r_sinogram, nb_clamp_sampler, sino_coord) + detected_value * fDistCorr;
    write_imagef (w_sinogram, sino_coord, det_value);

}

__kernel
void BP(__read_only  image2d_t           r_volume,
        __write_only image2d_t           w_volume,
        __read_only  image2d_t           sinogram,
        __const      float               relax_param,
        __constant   float               *sin_val,
        __constant   float               *cos_val,
        __const      float               axis_pos,
        __const      UfoProjectionsSubset    part)
{
    __const int2 vol_coord;
    vol_coord.x = get_global_id(0);
    vol_coord.y = get_global_id(1);
    
    __const float f_axis_pos = axis_pos;
    __const float fX = convert_float(vol_coord.x) + 0.5f - f_axis_pos;
    __const float fY = convert_float(vol_coord.y) + 0.5f - f_axis_pos;

    /*if (fX > f_axis_pos || fY > f_axis_pos)
        return;*/

    float4 value = 0.0f;
    float2 sino_coord;
    sino_coord.y = part.offset + 0.5f;

    for (int i = 0; i < part.n; ++i) {
        float sin_theta = sin_val[i + part.offset];
        float cos_theta = cos_val[i + part.offset];

        sino_coord.x = f_axis_pos + fX * cos_theta - fY * sin_theta;

        // the difference of ufo fbp and this fbp in sampling
        // if I will use linear_clamp_sampler instead linear_clamp_edge_sampler,
        // then I will got a circle on the reconstruction.
        // Otherwise application of this backprojection in FBP will lead to
        // the intensity increasing on the borders.

        value += read_imagef(sinogram, linear_clamp_edge_sampler, sino_coord);
        sino_coord.y += 1.0f;

    }
    value = read_imagef(r_volume, nb_clamp_sampler, vol_coord) + relax_param * value;
    write_imagef (w_volume, vol_coord, value);
}