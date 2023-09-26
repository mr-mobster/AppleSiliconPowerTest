#include "benchmark.h"

#include <mach/mach_time.h>
#include <assert.h>
#include <alloca.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>

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
    struct proc_threadcounts_data ptc_counts[2];
};

// 2 types of cores hardcoded (shoudl ideally look up hw.nperflevels)
static size_t threadcounts_size = sizeof(struct proc_threadcounts); // + 2 * sizeof(struct proc_threadcounts_data);

#define PROC_PIDTHREADCOUNTS 34

int proc_pidinfo(int pid, int flavor, uint64_t arg, void *buffer, int buffersize);



static double convert_mach_time(uint64_t mach_time) {
  static mach_timebase_info_data_t base = { .numer = 0 };
  if (base.numer == 0) mach_timebase_info(&base);
  
  double elapsed = (mach_time * base.numer) / base.denom;
  return elapsed / 1e9;
}

// version of the benchmark using threads
const size_t MAX_THREADS = 32;
typedef struct  {
  pthread_t thread;
  uint64_t  id;
  atomic_uint_fast64_t  items;
  atomic_uint_fast64_t  primes;
  struct proc_threadcounts counters;
} thread_data_t;
int threads_running = 0;

thread_data_t threads[MAX_THREADS] = { 0 };


void* test_thread(void* payload) {
  thread_data_t* thread = payload;
  const int max = 50000;
  
  atomic_store(&thread->items, 0);
  atomic_store(&thread->primes, 0);
  
  
  while (true) {
    pthread_testcancel();
    
    int i, num = 1;

    while (num <= max) {
      i = 2;
      while (i <= num) {
        if (num % i == 0)
          break;
        i++;
      }
      if (i == num) atomic_fetch_add(&thread->primes, 1);

      num++;
      
      atomic_fetch_add(&thread->items, 1);
    }
  }
}

void benchmark_start_threads(int n_threads) {
  pid_t pid = getpid();
  assert(threads_running == 0);
  
  // launch the threads
  for(int i = 0; i < n_threads; i++) {
    assert(threads[i].thread == 0);
    pthread_create(&threads[i].thread, NULL, &test_thread, threads + i);
    pthread_threadid_np(threads[i].thread, &threads[i].id);
  }
  
  // wait a little bit before sampling
  usleep(100);
  
  // sample all thread counters
  for(int i = 0; i < n_threads; i++) {
    int size = proc_pidinfo(pid, PROC_PIDTHREADCOUNTS, threads[i].id, &threads[i].counters, sizeof(struct proc_threadcounts));
    assert(size = sizeof(struct proc_threadcounts));
    
    atomic_store(&threads[i].items, 0);
    atomic_store(&threads[i].primes, 0);
    
    
  }
  
  threads_running = n_threads;
}

void benchmark_teardown_threads(void) {
  assert(threads_running > 0);
  
  // cancel all threads and wait for them to terminate
  for (int i = 0; i < threads_running; i++) {
    pthread_cancel(threads[i].thread);
  }

  for (int i = 0; i < threads_running; i++) {
    pthread_join(threads[i].thread, NULL);
    threads[i] = (thread_data_t) { 0 };
  }

  threads_running = 0;
}

void benchmark_sample_threads(int n_threads, benchmark_sample_t samples[n_threads]) {
  assert(n_threads > 0 && n_threads == threads_running);
  pid_t pid = getpid();
  struct proc_threadcounts* c0 = alloca(sizeof(struct proc_threadcounts));
  
  // sample all threads
  for(int i = 0; i < n_threads; i++) {
    *c0 = threads[i].counters;
    struct proc_threadcounts* c1 = &threads[i].counters;
    
    int size = proc_pidinfo(pid, PROC_PIDTHREADCOUNTS, threads[i].id, &threads[i].counters, sizeof(struct proc_threadcounts));
    assert(size = sizeof(struct proc_threadcounts));
    
    samples[i] = (benchmark_sample_t) {
      .thread_id = i,
      .items = atomic_exchange(&threads[i].items, 0),
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
    
    atomic_store(&threads[i].primes, 0);
  }
}

