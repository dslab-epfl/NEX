#pragma once
#include <sys/ptrace.h>
void ptrace_read(int pid, unsigned long addr, void *vptr, int len);
void ptrace_write(int pid, unsigned long addr, void *vptr, int len);