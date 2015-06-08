#ifndef PRL_MEM_H
#define PRL_MEM_H

//TODO: Avoid including this
#if defined(__APPLE__)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

struct prl_mem_struct;
typedef struct prl_mem_struct *prl_mem;

enum prl_mem_access {
    prl_mem_access_none = 0,
    prl_mem_access_read = 1,
    prl_mem_access_write = 2,
    prl_mem_access_rw = prl_mem_access_read | prl_mem_access_write
};
typedef enum prl_mem_access prl_mem_access;

void *prl_alloc(size_t size); // fixed
void prl_free(void *ptr);     // fixed

prl_mem *prl_mem_create(size_t size);
prl_mem *prl_mem_create_prefilled(size_t size, void *data);
prl_mem *prl_mem_create_memset(size_t size, char fillchar);
void prl_mem_host_access(prl_mem *mem, prl_mem_access access);
void prl_mem_device_access(prl_mem *mem, prl_mem_access access);
void prl_mem_set_host_mem(prl_mem *mem, void *host);
void prl_mem_opencl_set_device_mem(prl_mem *mem, cl_mem dev);

void *prl_mem_get_host_mem(prl_mem *mem);
cl_mem prl_mem_opencl_get_device(prl_mem *mem);

void prl_mem_free(prl_mem *mem);

#if defined(__cplusplus)
}
#endif

#endif /* PRL_MEM_H */
