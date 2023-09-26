#ifndef benchmark_h
#define benchmark_h

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  // cycles executed by the thread
  double cycles;
  // energy used by thread, J
  double energy;
  // sampling interval in seconds
  double time;
} cpu_counters_t;

typedef struct {
  // thread id
  int thread_id;
  // CPU counters for P and E core use
  cpu_counters_t p_core_counters;
  cpu_counters_t e_core_counters;
  // number of processed items
  uint64_t items;
  // is low power mode enabled
  bool low_power;
} benchmark_sample_t;


benchmark_sample_t run_benchmark(void);


void benchmark_sample_threads(int n_threads, benchmark_sample_t samples[n_threads]);
void benchmark_teardown_threads(void);
void benchmark_start_threads(int n_threads);

#endif /* benchmark_h */
