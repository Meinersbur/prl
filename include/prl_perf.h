#ifndef PRL_PERF_H
#define PRL_PERF_H

#if defined(__cplusplus)
extern "C" {
#endif

void prl_perf_reset();
void prl_perf_start();
void prl_perf_stop();
void prl_perf_dump();

typedef void (*timing_callback)(void *user);
void prl_perf_benchmark(timing_callback benched_func, timing_callback init_callback, timing_callback finit_callback, void *user);

#if defined(__cplusplus)
}
#endif

#endif /* PRL_PERF_H */
