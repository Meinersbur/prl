#ifndef PRL_H
#define PRL_H

#include "prl_scop.h"
#include "prl_mem.h"
#include "prl_perf.h"
#include "prl_opencl.h"

#if defined(__cplusplus)
extern "C" {
#endif

/// Initialize explicitly now
/// Do nothing if already initialized
/// Calling it is optional; without it, initialization happens on first time use
void prl_init();

/// Release all resources
/// Calling it is optional; without it, resources are freed on program exit
/// TODO: Option to only release resources on program exist when an option is set to satisfy leak detector tools
/// PRL will initialize again if it is used again or prl_init is called
void prl_release();

#if defined(__cplusplus)
}
#endif

#endif /* PRL_H */
