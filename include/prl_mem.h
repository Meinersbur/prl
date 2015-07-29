#ifndef PRL_MEM_H
#define PRL_MEM_H

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct prl_mem_struct;
typedef struct prl_mem_struct *prl_mem;

enum prl_mem_flags {
    // defaults, not necessary to state explicitely, but may make code more readable what is meant
    prl_mem_readable_writable = 0,
    prl_mem_readable = 0,
    prl_mem_writable = 0,
    prl_mem_host_readable = 0,
    prl_mem_host_writable = 0,
    prl_mem_dev_readable = 0,
    prl_mem_dev_writable = 0,

    // Transfer mechanism
    prl_mem_mechanism_default = 0,

    // Take ownership; i.e. free resource on prl_mem_free
    prl_mem_host_take = 1 << 0,
    prl_mem_dev_take = 1 << 1,

    // Whether the buffers are read/written in SCoPs (dev) or outside (host)
    prl_mem_host_noread = 1 << 2,
    prl_mem_host_nowrite = 1 << 3,
    prl_mem_host_noaccess = prl_mem_host_noread | prl_mem_host_nowrite,
    prl_mem_dev_noread = 1 << 4,
    prl_mem_dev_nowrite = 1 << 5,
    prl_mem_dev_noaccess = prl_mem_dev_noread | prl_mem_dev_nowrite,

    // Switch off automatic data transfers when SCoP finishes
    prl_mem_writeback_off = 1 << 6,    // Do not transfer data back to host
    prl_mem_writeback_nowait = 1 << 7, // Enqueue transfer, but do not wait for its completion when leaving the SCoP (conflicts prl_mem_writeback_off)

    // Where the most recent data currently resides (the initial data is taken from there)
    prl_mem_content_undefined = 0,
    prl_mem_content_hostside = 1 << 8,
    prl_mem_content_devside = 1 << 9,
    prl_mem_content_both = prl_mem_content_hostside | prl_mem_content_devside,

    // OpenCL specific (bits 16 upwards)
    prl_mem_opencl_dev_take = prl_mem_dev_take,
    prl_mem_opencl_mechanism_rwbuf = 1 << 16, // Use clEnqueueReadBuffer/clEnqueueWriteBuffer
    prl_mem_opencl_mechanism_map = 2 << 16,
    prl_mem_opencl_mechanism_svm = 3 << 16,
};

void *prl_alloc(size_t size); // fixed
void prl_free(void *ptr);     // fixed

prl_mem prl_mem_alloc(size_t size, enum prl_mem_flags flags); // Initial content is undefined
prl_mem prl_mem_alloc_prefill(size_t size, char fillchar, enum prl_mem_flags flags);
prl_mem prl_mem_alloc_preinit(size_t size, void *data, enum prl_mem_flags flags);
prl_mem prl_mem_manage_host(size_t size, void *host_ptr, enum prl_mem_flags flags);
void prl_mem_free(prl_mem mem);

//void *prl_mem_get_host_mem(prl_mem mem);
//cl_mem prl_mem_get_dev_mem(prl_mem mem);

#if defined(__cplusplus)
}
#endif

#endif /* PRL_MEM_H */
