kernel
void Dx (global float *input,
         global float *output)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int width = get_global_size(0);
    const int index = y * width + x;

    output[index] = input[index] - input[index - 1 + (x == 0 ? width : 0)];
}

kernel
void Dxt (global float *input,
          const int stopInd,
          global float *output)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int width = get_global_size(0);
    const int index = y * width + x;

    output[index] = input[index] - input[index + 1 - (x == stopInd ? width : 0)];
}

kernel
void Dy (global float *input,
         const int lastOffset,
         global float *output)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int width = get_global_size(0);
    const int index = y * width + x;

    output[index] = input[index] - input[index - width + (y == 0 ? lastOffset : 0)];
}

kernel
void Dyt (global float *input,
          const int lastOffset,
          const int stopInd,
          global float *output)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int width = get_global_size(0);
    const int index = y * width + x;

    output[index] = input[index] - input[index + width - (y == stopInd ? lastOffset : 0)];
}
