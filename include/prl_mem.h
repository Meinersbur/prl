#ifndef PRL_MEM_H
#define PRL_MEM_H

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct prl_mem_struct;
typedef struct prl_mem_struct *prl_mem;

enum prl_mem_flags {
    // defaults, not necessary to state explicitly, but may make code more readable what is meant.
    prl_mem_readable_writable = 0,
    prl_mem_readable = 0,
    prl_mem_writable = 0,
    prl_mem_host_readable = 0,
    prl_mem_host_writable = 0,
    prl_mem_dev_readable = 0,
    prl_mem_dev_writable = 0,

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
};

/* Prefer prl_mem_alloc over this */
void *prl_alloc(size_t size);
void prl_free(void *ptr);

/* Get the prl_mem object that manages the host pointer.
 * Must have been allocated before using prl_alloc or returned by prl_mem_get_host_mem. prl_get_mem returns zero if it is not a known host pointer.
 * Avoid using this, just use the prl_mem object returned by one of the prl_mem_alloc functions. */
prl_mem prl_get_mem(void *ptr);

prl_mem prl_mem_alloc(size_t size, enum prl_mem_flags flags); // Initial content is undefined
prl_mem prl_mem_alloc_prezero(size_t size, enum prl_mem_flags flags);
prl_mem prl_mem_alloc_prefill(size_t size, char fillchar, enum prl_mem_flags flags);
prl_mem prl_mem_alloc_preinit(size_t size, void *data, enum prl_mem_flags flags);

/* Use existing host address as host-side buffer.
 * May make the use of some OpenCL features impossible (like Shared Virtual Memory, SVM).
 * It is possible to to use this function on already managed host memory, in which case the size must match the previous allocation. */
prl_mem prl_mem_manage_host(size_t size, void *host_ptr, enum prl_mem_flags flags);

/* Free the memory allocated by one of the prl_mem_alloc functions, prl_mem_manage_host, or prl_get_mem(prl_alloc(size)). */
void prl_mem_free(prl_mem mem);

/* Return a dereferencable host pointer containing the current data.
 * Do not read from it while prl_mem_host_noread flag is set.
 * Do not write to it if prl_mem_host_nowrite is set.
 * Do not even call this function at all if both flags (prl_mem_host_noaccess) are set. */
void *prl_mem_get_host_mem(prl_mem mem);

void prl_mem_change_flags(prl_mem mem, enum prl_mem_flags add_flags, enum prl_mem_flags remove_flags);
void prl_mem_add_flags(prl_mem mem, enum prl_mem_flags add_flags);
void prl_mem_remove_flags(prl_mem mem, enum prl_mem_flags remove_flags);

/* Invalidate memory at this point. Equivalent of __pencil_kill in PENCIL. */
void prl_mem_kill(prl_mem mem);

/* Set all memory to zero. */
void prl_mem_zero(prl_mem mem);

/* Set all bytes in the buffer to the given character */
void prl_mem_fill(prl_mem mem, char fillchar);

#if defined(__cplusplus)
}
#endif

#endif /* PRL_MEM_H */
