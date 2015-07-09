#ifndef PRL_PERF_H
#define PRL_PERF_H

#if defined(__cplusplus)
extern "C" {
#endif

void prl_prof_reset();
void prl_prof_begin();
void prl_prof_end();
void prl_prof_dump();

typedef void (*timing_callback)(void *user);
void prl_prof_benchmark(timing_callback benched_func, void *user, timing_callback init_callback, void *init_user, timing_callback finit_callback, void *finit_user);

#if defined(__cplusplus)
}
#endif

#endif /* PRL_PERF_H */
