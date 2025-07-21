
#include <unistd.h>
#include <accvm/acctime.h>
#include <accvm/fileops.h>

int fsync(int fd){
    // uint64_t start_vts = read_vts();
    int ret = orig_fsync(fd);
    // uint64_t now_vts = read_vts();
    // uint64_t sd = slowdown();
    // uint64_t till = start_vts+(now_vts - start_vts)*sd;
    // // penalize_thread(getpid(), till);
    // // printf("=== fsync returned virtual %lu till  %lu\n", now_vts-start_vts, till);
    // while( now_vts < till ){
    //     // usleep((till-now_vts)/1000);
    //     now_vts = read_vts();
    // }
    return ret;
}