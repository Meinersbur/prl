/*
 * Copyright (c) 2014, ARM Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef IMPL_H
#define IMPL_H

#if defined (__APPLE__)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

#include "prl.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef  struct __int_pencil_cl_kernel *prl_cl_kernel;
    typedef  struct __int_pencil_cl_program *prl_cl_program;
    typedef  struct __int_pencil_cl_mem *prl_cl_mem;

    prl_cl_program __int_opencl_create_program_from_file (const char *, const char *);
    prl_cl_program __int_opencl_create_program_from_string (const char *, size_t, const char *);
    void __int_opencl_release_program (prl_cl_program);
    prl_cl_kernel __int_opencl_create_kernel (prl_cl_program,
                                                 const char *);
    void __int_opencl_release_kernel (prl_cl_kernel);
    prl_cl_mem __int_opencl_create_device_buffer (cl_mem_flags, size_t,
                                                     void *);
    void __int_opencl_release_buffer (prl_cl_mem);
    void __int_opencl_copy_to_device (prl_cl_mem, size_t, void *);
    void __int_opencl_copy_to_host (prl_cl_mem, size_t, void *);
    void *__int_pencil_alloc (size_t);
    void __int_pencil_free (void *);
    void __int_pencil_init (int n_devices, const cl_device_type * devices, bool cpu_profiling_enabled, bool gpu_profiling_enabled, bool blocking);
    void __int_pencil_shutdown  (bool print_stats_on_release, const char *prefix);
    void __int_opencl_set_kernel_arg (prl_cl_kernel, cl_uint, size_t,
                                      const void *, int);
    void __int_opencl_launch_kernel (prl_cl_kernel, cl_uint, const size_t *,
                                     const size_t *, const size_t *);
	
	void __int_pencil_dump_stats(const char *prefix);
	void __int_pencil_reset_stats();

	void __int_print_timings();
	void __int_reset_timings();
	void __int_pencil_timing_start();
	void __int_pencil_timing_stop();
	void __int_pencil_timing(timing_callback timed_func, void *user, timing_callback init_callback, void *init_user, timing_callback finit_callback, void *finit_user, enum prl_init_flags flags, int dryruns, int runs, const char *prefix);

#ifdef __cplusplus
}
#endif

#endif
