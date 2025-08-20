#include "exec/bpf.h"
#include <assert.h>
#include <bits/stdint-uintn.h>
#include <bits/types/struct_timeval.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>

#include <accvm/accvm.h>
#include <accvm/acctime.h>
#include <accvm/thread.h>
#include <accvm/network.h>
#include <accvm/context.h>
#include <accvm/decode.h>
#include <accvm/perf.h>

#include <drivers/jpegdecoder/jpeg_decoder_driver.h>
#include <drivers/jpegdecoder/jpeg_decoder_regs.h>
#include <drivers/vta/regs.h>
#include <drivers/driver_common.h>
#include <sims/sim.h>

#include <bpf/bpf.h>

#include <drivers/driver_common.h>
#include <errno.h>
#include <drivers/jpegdecoder/jpeg_decoder_driver.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

uintptr_t open_shm_for_nex(const char* shm_path) {
    printf("shm_path: %s\n", shm_path);
    // Use shm_open for POSIX shared memory objects.
    int fd = shm_open(shm_path, O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    // Get the size of the shared memory region.
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // printf("%s sb.st_size: %ld\n", shm_path, sb.st_size);
    size_t region_size = sb.st_size;
    if (region_size == 0) {
        fprintf(stderr, "Error: Shared memory region size is 0.\n");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    // Map the shared memory region into our address space.
    void *mmap_base = mmap(NULL, region_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmap_base == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    // printf("Memory mapped at address %p, size %zu bytes.\n", mmap_base, region_size);
    
    // The mapping remains valid even after closing fd.
    close(fd);
    
    return (uintptr_t)mmap_base;
}

uintptr_t driver_initialize(){
    const char *shm_path = "/nex_mmio_regions";
    
    // char* mmio_base_str = getenv("");
    // printf("str: %s\n", mmio_base_str);
    // uintptr_t mmio_base = (uintptr_t)strtoul(mmio_base_str, NULL, 0);
    // *(uint64_t*)(mmio_base) = 0x1000;
    return (uintptr_t)open_shm_for_nex(shm_path);
}

__attribute__((constructor)) static void accvm_init(void) {
  initialize_functions();
  ctrl_base = driver_initialize();
  vts_fd =  bpf_obj_get("/sys/fs/bpf/vts");
  if (vts_fd < 0) {
      fprintf(stderr, "Error getting BPF vts map\n");
      return;
  }

  bpf_sched_ctrl_fd = bpf_obj_get("/sys/fs/bpf/bpf_sched_ctrl");
  if (bpf_sched_ctrl_fd < 0) {
      fprintf(stderr, "Error getting BPF bpf_sched_ctrl map\n");
      return;
  }

  sim_proc_state_fd = bpf_obj_get("/sys/fs/bpf/sim_proc_state");

  if (sim_proc_state_fd < 0) {
      fprintf(stderr, "Error getting BPF sim_proc_state map\n");
      return;
  }
  
  // clean up everything 
  printf("LD_PRELOADED NEX Library \n");
  sys_up_time = get_real_ts();
}

__attribute__((destructor)) static void accvm_cleanup(void) {
}
