__kernel
void Dx(
    __global float *input, 
    __global float *output 
    )
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int width = get_global_size(0);
    const int index = y * width + x;

    output[index] = input[index] - input[index - 1 + (x == 0 ? width : 0)];
}

__kernel
void Dxt(
    __global float *input, 
    __const int stopInd,
    __global float *output
    )
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int width = get_global_size(0);
    const int index = y * width + x;
    
    output[index] = input[index] - input[index + 1 - (x == stopInd ? width : 0)];
}

__kernel
void Dy(
    __global float *input, 
    __const int lastOffset, 
    __global float *output
    )
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int width = get_global_size(0);
    const int index = y * width + x;

    output[index] = input[index] - input[index - width + (y == 0 ? lastOffset : 0)];
}

__kernel
void Dyt(
    __global float *input, 
    __const int lastOffset, 
    __const int stopInd, 
    __global float *output
    )
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int width = get_global_size(0);
    const int index = y * width + x;

    output[index] = input[index] - input[index + width - (y == stopInd ? lastOffset : 0)];
}