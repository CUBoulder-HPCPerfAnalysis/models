#define _XOPEN_SOURCE 600

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Timing information
typedef unsigned long long cycles_t;
cycles_t rdtsc() {
  unsigned hi, lo;
  __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
  return ((cycles_t)lo)|( ((cycles_t)hi)<<32);
}

static int usage(const char argv0[]) {
  fprintf(stderr,"Usage: %s [-s set_size] [-c cycles] app [args]\n",argv0);
  return 1;
}

int main(int argc,char *argv[]) {
  int opt,pid,verbose = 0;
  size_t set_size = 4096;        // Default to using one out of 8 ways in the 32 KiB cache.
  size_t cycles_per_loop = 1000; // ~4 bytes per cycle is a good DRAM memory bandwidth
  while ((opt = getopt(argc,argv,"+s:c:v"))) {
    if (opt == -1) break;
    switch (opt) {
    case 's':
      set_size = (size_t)atol(optarg);
      break;
    case 'c':
      cycles_per_loop = (size_t)atol(optarg);
      break;
    case 'v':
      verbose = 1;
      break;
    default:
      printf("Unexpected option: %d\n",opt);
      return usage(argv[0]);
    }
  }
  if ((pid = fork())) { // Parent
    int status;
    int32_t *buffer;
    posix_memalign((void**)&buffer,4096,set_size); /* 4 KiB page aligned so we fill cache sets in a deterministic way */
    while (!waitpid(pid,&status,WNOHANG)) {
      cycles_t cycles0 = rdtsc(),cycles_done;
      // Force this thread's working set into cache
      for (size_t i=0; i<set_size/sizeof(buffer[0]); i+=64/sizeof(buffer[0])) {
        buffer[i] = 0;
        __asm__ volatile("pause" ::: "memory");
      }
      cycles_done = rdtsc() - cycles0;
      if (verbose) cycles0 = rdtsc();
      // Agner says PAUSE has a latency of 5 cycles, but I measure 13-15 in the loop below.
      for (cycles_t i=0; cycles_done+i*16<cycles_per_loop; i++) __asm__ volatile("pause" ::: "memory");
      if (verbose) printf("cycles %llu %llu\n",cycles_done,16*(rdtsc()-cycles0)/(cycles_per_loop-cycles_done));
    }
    free(buffer);
  } else {
    execvp(argv[optind],argv+optind);
  }
  return 0;
}
