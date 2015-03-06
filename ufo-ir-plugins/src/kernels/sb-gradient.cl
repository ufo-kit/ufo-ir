__kernel
void Dx(__global float *input, int x, int y, int width, __global float *output)
{
    int index = y * width + x;
    output[index] = input[index] - input[index - 1 + (x == 0 ? width : 0)];
}

__kernel
void Dxt(__global float *input, int x, int y, int width, int stopInd, __global float *output)
{
    int index = y * width + x;
    output[index] = input[index] - input[index + 1 - (x == stopInd ? width : 0)];
}

__kernel
void Dy(__global float *input, int x, int y, int width, int lastOffset, __global float *output)
{
    int index = y * width + x;
    output[index] = input[index] - input[index - width + (y == 0 ? lastOffset : 0)];
}

__kernel
void Dyt(__global float *input, int x, int y, int width, int lastOffset, int stopInd, __global float *output)
{
    int index = y * width + x;
    output[index] = input[index] - input[index + width - (y == stopInd ? lastOffset : 0)];
}