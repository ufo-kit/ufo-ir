const sampler_t nb_clamp_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

__kernel
void shrinkage(__read_only  image2d_t input,
               __write_only image2d_t output,
                            float     psi)
{
    int2 coord;
    coord.x = get_global_id(0);
    coord.y = get_global_id(1);

    float4 vpsi;
    vpsi.s0 = psi;
    vpsi.s1 = vpsi.s2 = vpsi.s3 = 0;

    float4 zero = 0;

    float4 vx  = read_imagef(input, nb_clamp_sampler, coord);
    float4 result = native_divide(vx, fabs(vx)) * fmax (fabs(vx) - vpsi, zero);

    write_imagef (output, coord, result);
}