#include "benchmark.h"

#include <mach/mach_time.h>
#include <assert.h>
#include <alloca.h>
#include <pthread.h>
#include <unistd.h>

// internal sys/proc_info.h
//
// https://github.com/apple-oss-distributions/xnu/blob/aca3beaa3dfbd42498b42c5e5ce20a938e6554e5/bsd/sys/proc_info.h#L898
struct proc_threadcounts_data {
    uint64_t ptcd_instructions;
    uint64_t ptcd_cycles;
    uint64_t ptcd_user_time_mach;
    uint64_t ptcd_system_time_mach;
    uint64_t ptcd_energy_nj;
};

struct proc_threadcounts {
    uint16_t ptc_len;
    uint16_t ptc_reserved0;
    uint32_t ptc_reserved1;
    struct proc_threadcounts_data ptc_counts[];
};

// 2 types of cores hardcoded (shoudl ideally look up hw.nperflevels)
static size_t threadcounts_size = sizeof(struct proc_threadcounts) + 2 * sizeof(struct proc_threadcounts_data);

#define PROC_PIDTHREADCOUNTS 34

int proc_pidinfo(int pid, int flavor, uint64_t arg, void *buffer, int buffersize);



static double convert_mach_time(uint64_t mach_time) {
  static mach_timebase_info_data_t base = { .numer = 0 };
  if (base.numer == 0) mach_timebase_info(&base);
  
  double elapsed = (mach_time * base.numer) / base.denom;
  return elapsed / 1e9;
}


// brute-force prime numbers scan
int count_primes(int max) {
  int i, num = 1;
  int primes = 0;

  while (num <= max) {
    i = 2;
    while (i <= num) {
      if (num % i == 0)
        break;
      i++;
    }
    if (i == num)
      primes++;

    num++;
  }

  return primes;
}

// TODO: this should spawn a separate worker thread...
benchmark_sample_t run_benchmark(void) {
  // obtain the thread id
  pid_t pid = getpid();
  uint64_t tid;
  pthread_threadid_np(NULL, &tid);
   
  // allocate the counters
  struct proc_threadcounts* c0 = alloca(threadcounts_size);
  struct proc_threadcounts* c1 = alloca(threadcounts_size);
  
  
  // sample the counters before and after running work
  assert(proc_pidinfo(pid, PROC_PIDTHREADCOUNTS, tid, c0, (int) threadcounts_size) == threadcounts_size);
  uint64_t primes = count_primes(100000);
  assert(proc_pidinfo(pid, PROC_PIDTHREADCOUNTS, tid, c1, (int) threadcounts_size) == threadcounts_size);
  
  
  return (benchmark_sample_t) {
    .primes = primes,
    .p_core_counters = {
      .cycles = (c1->ptc_counts[0].ptcd_cycles - c0->ptc_counts[0].ptcd_cycles),
      .energy = (c1->ptc_counts[0].ptcd_energy_nj - c0->ptc_counts[0].ptcd_energy_nj)/1e9,
      .time   = convert_mach_time(c1->ptc_counts[0].ptcd_user_time_mach - c0->ptc_counts[0].ptcd_user_time_mach + c1->ptc_counts[0].ptcd_system_time_mach - c0->ptc_counts[0].ptcd_system_time_mach)
    },
    .e_core_counters = {
      .cycles = (c1->ptc_counts[1].ptcd_cycles - c0->ptc_counts[1].ptcd_cycles),
      .energy = (c1->ptc_counts[1].ptcd_energy_nj - c0->ptc_counts[1].ptcd_energy_nj)/1e9,
      .time   = convert_mach_time(c1->ptc_counts[1].ptcd_user_time_mach - c0->ptc_counts[1].ptcd_user_time_mach + c1->ptc_counts[1].ptcd_system_time_mach - c0->ptc_counts[1].ptcd_system_time_mach)
    }
  };
}

