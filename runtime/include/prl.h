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

#ifndef PRL_H
#define PRL_H

#include <stddef.h>
#include <stdlib.h>
#include <math.h>

#if defined(__APPLE__)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum prl_init_flags
{
    PRL_TARGET_DEVICE_DYNAMIC = 0, // Use as default
    PRL_TARGET_DEVICE_GPU_ONLY,
    PRL_TARGET_DEVICE_CPU_ONLY,
    PRL_TARGET_DEVICE_GPU_THEN_CPU,
    PRL_TARGET_DEVICE_CPU_THEN_GPU,

	PRL_CPU_PROFILING_ENABLED = 1<<3, // Skip the first three bits used to select the target device preference
	PRL_GPU_PROFILING_ENABLED = 1<<4,
	PRL_PROFILING_ENABLED = PRL_CPU_PROFILING_ENABLED | PRL_GPU_PROFILING_ENABLED,
	PRL_BLOCKING_ENABLED = 1 << 5,
};

/** Memory allocation. Can be used in the same way as malloc is used.  */
extern void *prl_alloc (size_t size);

/** Free memory allocated by prl_alloc.  */
extern void prl_free (void * ptr);

/** Initialize PRL runtime.  */
extern void prl_init (enum prl_init_flags flag);

/** Release PRL runtime.  */
extern void prl_shutdown (void);

typedef  struct __int_pencil_cl_kernel *prl_cl_kernel;
typedef  struct __int_pencil_cl_program *prl_cl_program;
typedef  struct __int_pencil_cl_mem *prl_cl_mem;

/* Create and compile OpenCL program from a file.  */
extern prl_cl_program prl_create_program_from_file (const char *, const char *);

/* Create and compile OpenCL program from a string.  */
extern prl_cl_program prl_create_program_from_string (const char *,
                                                            size_t,
                                                            const char *);

/* Releases the OpenCL program.  */
extern void prl_release_program (prl_cl_program);

/* Creates the kernel.  */
extern prl_cl_kernel prl_create_kernel (prl_cl_program, const char *);

/* Releases the kernel.  */
extern void prl_release_kernel (prl_cl_kernel);

/* Create device memory buffer.  */
extern prl_cl_mem prl_create_device_buffer (cl_mem_flags, size_t, void *);

/* Release the device memory buffer.  */
extern void prl_release_buffer (prl_cl_mem);

/* Copy host memory buffer to device memory.  */
extern void prl_copy_to_device (prl_cl_mem, size_t, void *);

/* Copy device memory buffer to host memory.  */
extern void prl_copy_to_host (prl_cl_mem, size_t, void *);

extern void prl_set_kernel_arg (prl_cl_kernel, cl_uint, size_t,
                                   const void *, int);

extern void prl_launch_kernel (prl_cl_kernel, cl_uint, const size_t *,
                                  const size_t *, const size_t *);


/** Dump ad-hoc statistics to stdout */
extern void prl_stats_dump(void);
extern void prl_stats_reset(void);

/* More accurate mesurement with multiple runs */
extern void prl_timings_start(void);
extern void prl_timings_stop(void);
extern void prl_timings_reset(void);
extern void prl_timings_dump(void);

typedef void (*timing_callback)(void *user);
extern void prl_timings(timing_callback timed_func, void *user, timing_callback init_callback, void *init_user, timing_callback finit_callback, void *finit_user);

/** smaller of two numbers, 
    potentially called by ppcg generated code */
static inline int __ppcg_min(int a, int b)
{
    if (a < b)
        return a;
    return b;
}

/** greater of two numbers, 
    potentially called by ppcg generated code */
static inline int __ppcg_max(int a, int b)
{
    if (a > b)
        return a;
    return b;
}

/** floored division (round to negative infinity), 
    potentially called by ppcg generated code      */
static inline int __ppcg_floord(int n, int d)
{
    if (n<0)
        return -((-n+d-1)/d);
    return n/d;
}



// Implementations for pencil_prototypes.h
// Required because ppcg may choose to generate CPU code for a PRL function
// pencil-optimizer will replicate these declarations, i.e. we cannot define them as macros
#include <pencil_lib.h>



#ifdef __cplusplus
}
#endif

#endif /* PRL_H */
