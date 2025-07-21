#include <exec/exec.h>
#include <exec/ptrace.h>
#include <exec/syscall.h>
#include <stdint.h>
#include <exec/thread_state.h>
#include <errno.h>
#include <time.h> 


// NOT USED
extern uint64_t read_vts();

void ns_to_timespec(uint64_t ns, struct timespec *tp) {
  tp->tv_sec = ns / 1000000000;
  tp->tv_nsec = ns % 1000000000;
}

void ns_to_timeval(uint64_t ns, struct timeval *tv) {
  tv->tv_sec = ns / 1000000000;
  tv->tv_usec = (ns % 1000000000)/1000;
}


void handle_clock_gettime(int child) {
    printf("Handling clock_gettime\n");
    uint64_t vts = read_vts();
    // this skips the syscall
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, child, NULL, &regs);
    regs.orig_rax = -1;
    regs.rax = 0;
    ptrace(PTRACE_SETREGS, child, NULL, &regs);

    long tp_addr = SYSCALL_ARG2_REG(regs);

    struct timespec tp = {0, 0};
    ns_to_timespec(vts, &tp);

    ptrace(PTRACE_POKEDATA, child, tp_addr, tp.tv_sec);
    ptrace(PTRACE_POKEDATA, child, tp_addr + sizeof(long), tp.tv_nsec);
    ptrace(PTRACE_CONT, child, 0, 0);
}

void handle_gettimeofday(int child){

}

void handle_clock_nanosleep(int child){

}

