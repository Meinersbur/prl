
int __ppcg_floord(int n, unsigned int d)
{
    if (n<0)
        return -(((unsigned int)(-n)+d-1)/d);
    return (unsigned int)n/d;
}

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void kernel0(__global double *data, __global double *mean, int m, int n)
{
    int b0 = get_group_id(0);
    int t0 = get_local_id(0);
    double private_mean[1];

    for (int c0 = 32 * b0; c0 < m; c0 += 1048576) {
      if (m >= t0 + c0 + 1) {
        private_mean[0] = 0.0;
        for (int c3 = 0; c3 <= min(31, n - 1); c3 += 1)
          private_mean[0] += data[c3 * n + (t0 + c0)];
        for (int c1 = 32; c1 < n; c1 += 32)
          for (int c3 = 0; c3 <= min(31, n - c1 - 1); c3 += 1)
            private_mean[0] += data[(c1 + c3) * n + (t0 + c0)];
        mean[(t0 + c0)] = private_mean[0];
      }
      barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
    }
}
__kernel void kernel1(__global double *stddev, int m, int n)
{
    int b0 = get_group_id(0);
    int t0 = get_local_id(0);

    for (int c0 = 32 * b0; c0 < m; c0 += 1048576)
      if (m >= t0 + c0 + 1)
        stddev[t0 + c0] = 0.0;
}
__kernel void kernel2(__global double *data, double float_n, __global double *mean, __global double *stddev, int m, int n)
{
    int b0 = get_group_id(0);
    int t0 = get_local_id(0);
    __local double shared_data[32][32];
    double private_mean[1];
    double private_stddev[1];

    for (int c0 = 32 * b0; c0 < m; c0 += 1048576) {
      if (m >= t0 + c0 + 1) {
        private_mean[0] = mean[(t0 + c0)];
        if (n >= 1)
          private_stddev[0] = stddev[(t0 + c0)];
      }
      for (int c1 = 0; c1 < n; c1 += 32) {
        if (n >= t0 + c0 + 1)
          for (int c2 = 0; c2 <= min(31, n - c1 - 1); c2 += 1)
            shared_data[c2][t0] = data[((c1 + c2) * n + t0 + c0)];
        barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
        if (m >= t0 + c0 + 1 && c1 == 0)
          private_mean[0] /= float_n;
        if (m >= t0 + c0 + 1)
          for (int c3 = 0; c3 <= min(31, n - c1 - 1); c3 += 1) {
            private_stddev[0] += ((shared_data[c3][t0] - private_mean[0]) * (shared_data[c3][t0] - private_mean[0]));
            shared_data[c3][t0] -= private_mean[0];
          }
        barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
        if (m >= t0 + c0 + 1)
          for (int c2 = 0; c2 <= min(31, n - c1 - 1); c2 += 1)
            data[((c1 + c2) * n + t0 + c0)] = shared_data[c2][t0];
      }
      if (n == 0) {
        barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
        if (m >= t0 + c0 + 1)
          private_mean[0] /= float_n;
        barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
      } else if (m >= t0 + c0 + 1)
        stddev[(t0 + c0)] = private_stddev[0];
      if (m >= t0 + c0 + 1)
        mean[(t0 + c0)] = private_mean[0];
      barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
    }
}
__kernel void kernel3(double float_n, __global double *stddev, int m, int n)
{
    int b0 = get_group_id(0);
    int t0 = get_local_id(0);

    for (int c0 = 32 * b0; c0 < m; c0 += 1048576)
      if (m >= t0 + c0 + 1)
        stddev[t0 + c0] /= float_n;
}
__kernel void kernel4(double eps, __global double *stddev, int m, int n)
{
    int b0 = get_group_id(0);
    int t0 = get_local_id(0);
    double private_stddev[1];

    for (int c0 = 32 * b0; c0 < m; c0 += 1048576)
      if (m >= t0 + c0 + 1) {
        private_stddev[0] = stddev[(t0 + c0)];
        private_stddev[0] = sqrt(private_stddev[0]);
        private_stddev[0] = ((private_stddev[0] <= eps) ? 1.0 : private_stddev[0]);
        stddev[(t0 + c0)] = private_stddev[0];
      }
}
__kernel void kernel5(__global double *data, double float_n, __global double *stddev, int m, int n)
{
    int b0 = get_group_id(0), b1 = get_group_id(1);
    int t0 = get_local_id(0), t1 = get_local_id(1);
    __local double shared_stddev[32];

    for (int c0 = 32 * b0; c0 < n; c0 += 8192)
      for (int c1 = 32 * b1; c1 < m; c1 += 8192) {
        if (t0 == 0)
          for (int c2 = t1; c2 <= min(31, m - c1 - 1); c2 += 16)
            shared_stddev[c2] = stddev[(c1 + c2)];
        barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
        if (n >= t0 + c0 + 1)
          for (int c3 = t1; c3 <= min(31, m - c1 - 1); c3 += 16)
            data[(t0 + c0) * n + (c1 + c3)] /= (sqrt(float_n) * shared_stddev[c3]);
        barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
      }
}
__kernel void kernel6(__global double *data, __global double *symmat, int m, int n)
{
    int b0 = get_group_id(0), b1 = get_group_id(1);
    int t0 = get_local_id(0), t1 = get_local_id(1);
    __local double shared_data_0[32][32];
    double private_symmat[1][2];

    for (int c0 = 32 * b0; c0 < m - 1; c0 += 8192)
      for (int c1 = 32 * b1 + 8192 * (int)((unsigned int)(-32 * b1 + c0 + 8161) / 8192); c1 < m; c1 += 8192) {
        for (int c2 = 0; c2 < n; c2 += 32) {
          if (n >= t0 + c2 + 1)
            for (int c4 = t1; c4 <= min(31, n - c0 - 1); c4 += 16)
              shared_data_0[t0][c4] = data[((t0 + c2) * n + c0 + c4)];
          barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
          if (m >= t0 + c0 + 2 && m >= t1 + c1 + 1 && t1 + c1 + 15 >= t0 + c0 && c2 == 0) {
            if (t1 + c1 >= t0 + c0 + 1)
              private_symmat[0][0] = 0.0;
            if (m >= t1 + c1 + 17)
              private_symmat[0][1] = 0.0;
          }
          if (m >= t0 + c0 + 2 && m >= t1 + c1 + 1 && c1 + 30 >= (int)((unsigned int)(15 * t0 + t1 + 15) % 16) + t0 + c0)
            for (int c3 = 0; c3 <= min(31, n - c2 - 1); c3 += 1) {
              if (t1 + c1 >= t0 + c0 + 1)
                private_symmat[0][0] += (shared_data_0[c3][t0] * data[(c2 + c3) * n + (t1 + c1)]);
              if (m >= t1 + c1 + 17)
                private_symmat[0][1] += (shared_data_0[c3][t0] * data[(c2 + c3) * n + (t1 + c1 + 16)]);
            }
          barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
        }
        if (n == 0) {
          barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
          if (m >= t0 + c0 + 2 && m >= t1 + c1 + 1 && t1 + c1 + 15 >= t0 + c0) {
            if (t1 + c1 >= t0 + c0 + 1)
              private_symmat[0][0] = 0.0;
            if (m >= t1 + c1 + 17)
              private_symmat[0][1] = 0.0;
          }
          barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
        }
        if (m >= t0 + c0 + 2 && m >= t1 + c1 + 1 && t1 + c1 + 15 >= t0 + c0) {
          if (t1 + c1 >= t0 + c0 + 1)
            symmat[((t0 + c0) * (m >= 2 ? m : 1) + t1 + c1)] = private_symmat[0][0];
          if (m >= t1 + c1 + 17)
            symmat[((t0 + c0) * (m >= 2 ? m : 1) + t1 + c1 + 16)] = private_symmat[0][1];
        }
        barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
      }
}
__kernel void kernel7(__global double *symmat, int m, int n)
{
    int b0 = get_group_id(0), b1 = get_group_id(1);
    int t0 = get_local_id(0), t1 = get_local_id(1);
    __local double shared_symmat_0[32][32];

    for (int c0 = 32 * b0; c0 < m - 1; c0 += 8192)
      for (int c1 = 32 * b1 + 8192 * (int)((unsigned int)(-32 * b1 + c0 + 8161) / 8192); c1 < m; c1 += 8192) {
        barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
        for (int c3 = max(t1, t1 + 16 * __ppcg_floord(t0 - t1 + c0 - c1, 16) + 16); c3 <= min(31, m - c1 - 1); c3 += 16)
          shared_symmat_0[c3][t0] = symmat[(t0 + c0) * (m >= 2 ? m : 1) + (c1 + c3)];
        barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
        if (m >= t0 + c1 + 1)
          for (int c3 = t1; c3 <= min(31, t0 - c0 + c1 - 1); c3 += 16)
            symmat[((t0 + c1) * (m >= 2 ? m : 1) + c0 + c3)] = shared_symmat_0[t0][c3];
      }
}
__kernel void kernel8(__global double *symmat, int m, int n)
{
    int b0 = get_group_id(0);
    int t0 = get_local_id(0);

    for (int c0 = 32 * b0; c0 < m - 1; c0 += 1048576)
      if (m >= t0 + c0 + 2)
        symmat[(t0 + c0) * (m >= 2 ? m : 1) + (t0 + c0)] = 1.0;
}
