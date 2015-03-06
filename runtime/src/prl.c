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

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "prl.h"
#include "prl_impl.h"

static const char * env_name = "PENCIL_TARGET_DEVICE";
static const char * GPU_TARGET_DEVICE = "gpu";
static const char * CPU_TARGET_DEVICE = "cpu";
static const char * GPU_CPU_TARGET_DEVICE = "gpu_cpu";
static const char * CPU_GPU_TARGET_DEVICE = "cpu_gpu";
static const char * PRL_PROFILING = "PRL_PROFILING";
static const char * PRL_CPU_PROFILING = "PRL_CPU_PROFILING";
static const char * PRL_GPU_PROFILING = "PRL_GPU_PROFILING";
static const char * PRL_PROFILING_PREFIX = "PRL_PROFILING_PREFIX";
static const char * PRL_BLOCKING = "PRL_BLOCKING";
static const char * PRL_TIMINGS_RUNS = "PRL_TIMINGS_RUNS";
static const char * PRL_TIMINGS_DRY_RUNS = "PRL_TIMINGS_DRY_RUNS";
static const char * PRL_TIMINGS_PREFIX = "PRL_TIMINGS_PREFIX";
static const char * PRL_PREFIX = "PRL_PREFIX";

prl_cl_program prl_create_program_from_file (const char *filename,
                                                   const char *opts)
{
    return __int_opencl_create_program_from_file (filename, opts);
}

prl_cl_program prl_create_program_from_string (const char *program,
                                                     size_t size,
                                                     const char *opts)
{
    return __int_opencl_create_program_from_string (program, size, opts);
}

void prl_release_program (prl_cl_program program)
{
    return __int_opencl_release_program (program);
}

prl_cl_kernel prl_create_kernel (prl_cl_program program, const char *name)
{
    return __int_opencl_create_kernel (program, name);
}

void prl_release_kernel (prl_cl_kernel kernel)
{
    return __int_opencl_release_kernel (kernel);
}

prl_cl_mem prl_create_device_buffer (cl_mem_flags flags, size_t size,
                                    void *host_ptr)
{
    return __int_opencl_create_device_buffer (flags, size, host_ptr);
}

void prl_release_buffer (prl_cl_mem buffer)
{
    return __int_opencl_release_buffer (buffer);
}

void prl_copy_to_device (prl_cl_mem dev, size_t size, void *host)
{
    return __int_opencl_copy_to_device (dev, size, host);
}

void prl_copy_to_host (prl_cl_mem dev, size_t size, void *host)
{
    return __int_opencl_copy_to_host (dev, size, host);
}

void *prl_alloc (size_t size)
{
    return __int_pencil_alloc (size);
}

void prl_free (void *ptr)
{
    return __int_pencil_free (ptr);
}

static enum prl_init_flags check_environment ()
{
    const char * target_device = getenv(env_name);
    if (!target_device)
    {
        return PRL_TARGET_DEVICE_DYNAMIC;
    }
    if (!strcmp (target_device, GPU_TARGET_DEVICE))
    {
        return PRL_TARGET_DEVICE_GPU_ONLY;
    }
    if (!strcmp (target_device, CPU_TARGET_DEVICE))
    {
        return PRL_TARGET_DEVICE_CPU_ONLY;
    }
    if (!strcmp (target_device, GPU_CPU_TARGET_DEVICE))
    {
        return PRL_TARGET_DEVICE_GPU_THEN_CPU;
    }
    if (!strcmp (target_device, CPU_GPU_TARGET_DEVICE))
    {
        return PRL_TARGET_DEVICE_CPU_THEN_GPU;
    }
    return PRL_TARGET_DEVICE_DYNAMIC;
}

void prl_init (enum prl_init_flags flag)
{
	bool profiling_print =  getenv(PRL_PROFILING);
	bool cpu_profiling_print = profiling_print || getenv(PRL_CPU_PROFILING);
	bool gpu_profiling_print = profiling_print || getenv(PRL_GPU_PROFILING);

	bool cpu_profiling_enabled = cpu_profiling_print || (flag&PRL_CPU_PROFILING_ENABLED);
	bool gpu_profiling_enabled = gpu_profiling_print || (flag&PRL_GPU_PROFILING_ENABLED);
	bool blocking_enabled = getenv(PRL_BLOCKING) || (flag&PRL_BLOCKING_ENABLED);

	enum prl_init_flags device_flag = flag & (PRL_TARGET_DEVICE_GPU_ONLY | PRL_TARGET_DEVICE_CPU_ONLY | PRL_TARGET_DEVICE_GPU_THEN_CPU | PRL_TARGET_DEVICE_CPU_THEN_GPU | PRL_TARGET_DEVICE_DYNAMIC);
    static const cl_device_type gpu_only[] = {CL_DEVICE_TYPE_GPU};
    static const cl_device_type cpu_only[] = {CL_DEVICE_TYPE_CPU};
    static const cl_device_type gpu_cpu[] = {CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_CPU};
    static const cl_device_type cpu_gpu[] = {CL_DEVICE_TYPE_CPU, CL_DEVICE_TYPE_GPU};
    if (device_flag == PRL_TARGET_DEVICE_DYNAMIC)
    {
    	device_flag = check_environment ();
    }
    switch (device_flag)
    {
        case PRL_TARGET_DEVICE_GPU_ONLY: return __int_pencil_init (1, gpu_only, cpu_profiling_enabled, gpu_profiling_enabled, blocking_enabled);
        case PRL_TARGET_DEVICE_CPU_ONLY: return __int_pencil_init (1, cpu_only, cpu_profiling_enabled, gpu_profiling_enabled, blocking_enabled);
        case PRL_TARGET_DEVICE_GPU_THEN_CPU: return __int_pencil_init (2, gpu_cpu, cpu_profiling_enabled, gpu_profiling_enabled, blocking_enabled);
        case PRL_TARGET_DEVICE_CPU_THEN_GPU: return __int_pencil_init (2, cpu_gpu, cpu_profiling_enabled, gpu_profiling_enabled, blocking_enabled);
        case PRL_TARGET_DEVICE_DYNAMIC: return __int_pencil_init (0, NULL, cpu_profiling_enabled, gpu_profiling_enabled, blocking_enabled);
        default: assert(0);
    }
}

void prl_shutdown ()
{
	bool profiling_print =  getenv(PRL_PROFILING) || getenv(PRL_CPU_PROFILING) || getenv(PRL_GPU_PROFILING);
	const char *prefix = getenv(PRL_PROFILING_PREFIX);
	if (!prefix)
		prefix = getenv(PRL_PREFIX);
    return __int_pencil_shutdown (profiling_print, prefix);
}


void prl_set_kernel_arg (prl_cl_kernel kernel, cl_uint idx, size_t size,
                            const void *value, int buffer)
{
    return __int_opencl_set_kernel_arg (kernel, idx, size, value, buffer);
}

void prl_launch_kernel (prl_cl_kernel kernel, cl_uint work_dim,
                           const size_t *goffset, const size_t *gws,
                           const size_t *lws)
{
    return __int_opencl_launch_kernel (kernel, work_dim, goffset, gws, lws);
}


void prl_stats_dump (void) {
	const char *prefix = getenv(PRL_PROFILING_PREFIX);
	if (!prefix)
		prefix = getenv(PRL_PREFIX);
	__int_pencil_dump_stats(prefix);
}

void prl_stats_reset (void) {
	__int_pencil_reset_stats();
}


 void prl_timings_start(void) {
	 __int_pencil_timing_start();
 }
 void prl_timings_stop(void) {
	__int_pencil_timing_stop();
}

 void prl(void) {
	__int_reset_timings();
}
 void prl_timings_dump(void) {
	 const char *prefix = getenv(PRL_TIMINGS_PREFIX);
	 if (!prefix)
		 prefix = getenv(PRL_TIMINGS_PREFIX);

	 __int_print_timings(prefix);
 }


 void prl_timings(timing_callback timed_func, void *user, timing_callback init_callback, void *init_user, timing_callback finit_callback, void *finit_user) {
	 int dryruns = 2;
	 const char *sdryruns = getenv(PRL_TIMINGS_DRY_RUNS);
	 if (sdryruns) {
		 dryruns = strtol(sdryruns,NULL,10);
	 }

	 int runs = 30;
	 const char *sruns = getenv(PRL_TIMINGS_RUNS);
	 if (sruns) {
		 runs = strtol(sruns,NULL,10);
	 }

	 const char *prefix = getenv(PRL_TIMINGS_PREFIX);
	 if (!prefix)
		 prefix = getenv(PRL_PREFIX);

	 __int_pencil_timing(timed_func, user, init_callback, init_user, finit_callback, finit_user, PRL_TARGET_DEVICE_DYNAMIC, dryruns, runs, prefix);
}
