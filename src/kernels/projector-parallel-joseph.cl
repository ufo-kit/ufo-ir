const sampler_t nb_clamp_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
const sampler_t linear_clamp_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;

#define BLOCK_SIZE 64
#define UFO_BUFFER_MAX_NDIMS 3

typedef struct {
    long origin[UFO_BUFFER_MAX_NDIMS];
    long size[UFO_BUFFER_MAX_NDIMS];
} UfoRegion;

typedef struct {
  unsigned long height;
  unsigned long width;

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
            __read_only     image2d_t               r_sinogram,
            __write_only    image2d_t               w_sinogram,
            __constant      float                   *sin_val,
            __constant      float                   *cos_val,
            __const         UfoGeometryDims         dimensions,
            __const         float                   axis_pos,
            __const         UfoProjectionsSubset    part,
            __const         float                   correction_scale)
{
    int2 sino_coord;
    sino_coord.y = part.offset + get_global_id(1);
    sino_coord.x = get_global_id(0);

    float required_width = axis_pos * 2;

    // diff > 0 when the center of rotation is right of the sinogram center
    // and is left otherwise
    float diff = (float)dimensions.width - required_width;

    // Shift of the rotation center from the center of a slice in the X-axis
    float rotation_origin_shift = diff / 2.0f;
    // Shift to put the origin in X-axis into the center of the slice
    float origin_shift = (float)dimensions.width / 2.0f;

    // Switch off inactive detectors.
    if ((diff < 0 && sino_coord.x < fabs(diff)) ||
        (diff > 0 && sino_coord.x > required_width)) {
        return;
    }

    __const float fDetStep   = -1.0f / sin_val[sino_coord.y];
    float fSliceStep = cos_val[sino_coord.y] / sin_val[sino_coord.y];

    float2 volume_coord;
    volume_coord.x = 0.5f;
    volume_coord.y = (0.5 + rotation_origin_shift + sino_coord.x - 0.5f * dimensions.n_dets) * fDetStep +
                     (-origin_shift) * fSliceStep +
                     0.5f * dimensions.height;

    // split up the calculation by parts to increse percision
    float4 detected_value = 0.0f;
    float4 inner_sum;

    int    n_blocks  = ceil(convert_float(dimensions.width) / BLOCK_SIZE);
    for (int j = 0; j < n_blocks; ++j) {
        inner_sum = 0.0f;
        for (int i = 0; i < BLOCK_SIZE; i++) {
            inner_sum += read_imagef(volume, linear_clamp_sampler, volume_coord);
            volume_coord.y += fSliceStep;
            volume_coord.x += 1.0f;
        }
        detected_value += inner_sum;
    }

    float4 det_value = read_imagef(r_sinogram, nb_clamp_sampler, sino_coord) +
                       detected_value * correction_scale;
    write_imagef (w_sinogram, sino_coord, det_value);
}

__kernel
void FP_vert(__read_only     image2d_t               volume,
             __read_only     image2d_t               r_sinogram,
             __write_only    image2d_t               w_sinogram,
             __constant      float                   *sin_val,
             __constant      float                   *cos_val,
             __const         UfoGeometryDims         dimensions,
             __const         float                   axis_pos,
             __const         UfoProjectionsSubset    part,
             __const         float                   correction_scale)
{
    int2 sino_coord;
    sino_coord.y = part.offset + get_global_id(1);
    sino_coord.x = get_global_id(0);

    float required_width = axis_pos * 2;

    // diff > 0 when the center of rotation is right of the sinogram center
    // and is left otherwise
    float diff = (float)dimensions.width - required_width;

    // Shift of the rotation center from the center of a slice in the X-axis
    float rotation_origin_shift = diff / 2.0f;
    // Shift to put the origin in X-axis into the center of the slice
    float origin_shift = (float)dimensions.width / 2.0f;

    // Switch off inactive detectors.
    if ((diff < 0 && sino_coord.x < fabs(diff)) ||
        (diff > 0 && sino_coord.x > required_width)) {
        return;
    }

    __const float fDetStep   = 1.0f / cos_val[sino_coord.y];
    float fSliceStep = sin_val[sino_coord.y] / cos_val[sino_coord.y];

    float2 volume_coord;
    volume_coord.y = 0.5f;
    volume_coord.x = (0.5 + rotation_origin_shift + sino_coord.x - 0.5f * dimensions.n_dets) * fDetStep +
                     (-origin_shift) * fSliceStep +
                     0.5f * dimensions.width;

    // split up the calculation by parts to increse percision
    float4 detected_value = 0.0f;
    float4 inner_sum;

    int    n_blocks  = ceil(convert_float(dimensions.width) / BLOCK_SIZE);
    for (int j = 0; j < n_blocks; ++j) {
        inner_sum = 0.0f;
        for (int i = 0; i < BLOCK_SIZE; i++) {
            inner_sum += read_imagef(volume, linear_clamp_sampler, volume_coord);
            volume_coord.x += fSliceStep;
            volume_coord.y += 1.0f;
        }
        detected_value += inner_sum;
    }

    float4 det_value = read_imagef(r_sinogram, nb_clamp_sampler, sino_coord) +
                       detected_value * correction_scale;
    write_imagef (w_sinogram, sino_coord, det_value);
}

__kernel
void BP(__read_only  image2d_t           r_volume,
        __write_only image2d_t           w_volume,
        __read_only  image2d_t           sinogram,
        __const      float               relax_param,
        __constant   float               *sin_val,
        __constant   float               *cos_val,
        __const      UfoGeometryDims     dimensions,
        __const      float               axis_pos,
        __const      UfoProjectionsSubset    part)
{
    __const int2 vol_coord;
    vol_coord.x = get_global_id(0);
    vol_coord.y = get_global_id(1);


    float required_width = axis_pos * 2;

    // diff > 0 when the center of rotation is right of the sinogram center
    // and is left otherwise
    float diff = (float)dimensions.width - required_width;

    float half_active_dets = diff < 0 ? dimensions.width - axis_pos : axis_pos;
    float sino_edge_0 = axis_pos - half_active_dets + 0.5f;
    float sino_edge_1 = axis_pos + half_active_dets - 0.5f;

    // Shift of the rotation center from the center of a slice in the X-axis
    float rotation_origin_shift = diff / 2.0f;
    // Shift to put the origin in X-axis into the center of the slice
    float origin_shift = (float)dimensions.width / 2.0f;

    __const float fX = convert_float(vol_coord.x) + 0.5f - origin_shift;
    __const float fY = convert_float(vol_coord.y) + 0.5f - origin_shift;

    float4 value = 0.0f;
    float2 sino_coord;
    sino_coord.y = part.offset + 0.5f;

    for (int i = 0; i < part.n; ++i) {
        float sin_theta = sin_val[i + part.offset];
        float cos_theta = cos_val[i + part.offset];

        sino_coord.x = fX * cos_theta - fY * sin_theta +
                       (origin_shift - rotation_origin_shift);

        // Simulate clamp to edge addressing. It is simulated since
        // the standard opencl sampler will take inactive detectors
        // into account, which will lead to the incorrect reconstruction.
        if (sino_coord.x > sino_edge_1) {
          sino_coord.x = sino_edge_1;
        } else if (sino_coord.x < sino_edge_0) {
          sino_coord.x = sino_edge_0;
        }

        value += read_imagef(sinogram, linear_clamp_sampler, sino_coord);
        sino_coord.y += 1.0f;
    }

    value = read_imagef(r_volume, nb_clamp_sampler, vol_coord) + relax_param * value;
    write_imagef (w_volume, vol_coord, value);
}
