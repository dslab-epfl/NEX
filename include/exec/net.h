#include <sys/prctl.h>
#include <syscall.h> 
#include <linux/seccomp.h>
#include <linux/filter.h>
#include <stddef.h>
#include <sys/reg.h>
#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <exec/evnt.h>

void handle_netsyscall(int nr_call, int waited_child);