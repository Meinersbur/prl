#ifndef PRL_OPENCL_H
#define PRL_OPENCL_H

#include <prl_mem.h>

#if 0
//TODO: Avoid including this
//FIXME: ppcg even fails if this is included
#if defined(__APPLE__)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif
#else
struct _cl_mem;
typedef struct _cl_mem * cl_mem;

struct _cl_context;
typedef struct _cl_context * cl_context;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

cl_context prl_opencl_get_context();

prl_mem prl_opencl_mem_manage_dev(size_t size,                 cl_mem dev_ptr, enum prl_mem_flags flags);
prl_mem prl_opencl_mem_manage    (size_t size, void *host_ptr, cl_mem dev_ptr, enum prl_mem_flags flags);

#if defined(__cplusplus)
}
#endif

#endif /* PRL_OPENCL_H */
