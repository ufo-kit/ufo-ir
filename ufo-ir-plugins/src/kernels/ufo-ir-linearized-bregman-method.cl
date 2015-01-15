const sampler_t nb_clamp_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

__kernel
void gradient (__read_only  image2d_t input,
               __write_only image2d_t output)
{
  int2 coord;
  coord.x = get_global_id(0);
  coord.y = get_global_id(1);

  int width = get_global_size (0);
  int height = get_global_size (1);

  if (coord.x == 0) {
    float4 v_cur = read_imagef(input, nb_clamp_sampler, coord);

    coord.x+=1;
    float4 v_next = read_imagef(input, nb_clamp_sampler, coord);

    float4 res = v_next - v_cur;

    coord.x-=1;
    write_imagef(output, coord, res);
    return;
  }

  if (coord.x == (width - 1)) {
    float4 v_cur = read_imagef(input, nb_clamp_sampler, coord);

    coord.x-=1;
    float4 v_prev = read_imagef(input, nb_clamp_sampler, coord);

    float4 res = v_cur - v_prev;

    coord.x+=1;
    write_imagef(output, coord, res);
    return;
  }

  coord.x+=1;
  float4 v_next = read_imagef(input, nb_clamp_sampler, coord);
  coord.x-=2;
  float4 v_prev = read_imagef(input, nb_clamp_sampler, coord);
  float4 res = v_next - v_prev;
  coord.x+=1;

  write_imagef(output, coord, res);
}

__kernel
void computeIx (__read_only  image2d_t x,
                __write_only image2d_t output)
{
  int2 coord;
  coord.x = get_global_id(0);
  coord.y = get_global_id(1);

  float4 val;
  if (coord.x == coord.y) {
    val = read_imagef(x, nb_clamp_sampler, coord);
  } else {
    val = 0;
  }

  write_imagef(output, coord, val);
}














/*
__kernel
void transBx (__read_only  image2d_t input,
              __write_only image2d_t output)
{
  int2 coord;
  coord.x = get_global_id(0);
  coord.y = get_global_id(1);

  int width = get_global_size (0);
  int height = get_global_size (1);

  if (coord.x == 0) {
    float4 v_cur = read_imagef(input, nb_clamp_sampler, coord);

    coord.x+=1;
    float4 v_next = read_imagef(input, nb_clamp_sampler, coord);

    float4 res = v_next - v_cur;

    if (coord.x == coord.y)
      res -= v_cur;

    coord.x-=1;
    write_imagef(output, coord, res);
    return;
  }

  if (coord.x == width - 1) {
    float4 v_cur = read_imagef(input, nb_clamp_sampler, coord);

    coord.x-=1;
    float4 v_prev = read_imagef(input, nb_clamp_sampler, coord);

    float4 res = v_cur - v_prev;

    if (coord.x == coord.y)
      res -= v_cur;

    coord.x+=1;
    write_imagef(output, coord, res);
    return;
  }

  coord.x+=1;
  float4 v_next = read_imagef(input, nb_clamp_sampler, coord);
  coord.x-=2;
  float4 v_prev = read_imagef(input, nb_clamp_sampler, coord);
  float4 res = v_next - v_prev;

  coord.x += 1;
  if (coord.x == coord.y) {
      float4 v_cur = read_imagef(input, nb_clamp_sampler, coord);
      res -= v_cur;
  }

  write_imagef(output, coord, res);
}

__kernel
void transB (__read_only  image2d_t grad_u,
             __read_only  image2d_t p,
             __write_only image2d_t output)
{
  int2 coord;
  coord.x = get_global_id(0);
  coord.y = get_global_id(1);
  if (coord.x == coord.y) {
      float4 val = read_imagef(grad_u, nb_clamp_sampler, coord);
      val = val - read_imagef(p, nb_clamp_sampler, coord);
      write_imagef(output, coord, val);
  }
}
*/