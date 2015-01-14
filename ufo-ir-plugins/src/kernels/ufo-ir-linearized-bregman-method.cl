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

    coord.x++;
    float4 v_next = read_imagef(input, nb_clamp_sampler, coord);

    float4 res = v_next - v_cur;

    coord.x--;
    write_imagef(output, coord, res);
    return;
  }

  if (coord.x == width - 1) {
    float4 v_cur = read_imagef(input, nb_clamp_sampler, coord);

    coord.x--;
    float4 v_prev = read_imagef(input, nb_clamp_sampler, coord);

    float4 res = v_cur - v_prev;

    coord.x++;
    write_imagef(output, coord, res);
    return;
  }

  coord.x+=1;
  float4 v_next = read_imagef(input, nb_clamp_sampler, coord);
  coord.x-=2;
  float4 v_prev = read_imagef(input, nb_clamp_sampler, coord);
  float4 res = v_next - v_prev;
  write_imagef(output, coord, res);
}

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