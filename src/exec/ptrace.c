#include <exec/exec.h>
#include <exec/ptrace.h>

/*
 * ptrace_read()
 *
 * Use ptrace() to read the contents of a target process' address space.
 *
 * args:
 * - int pid: pid of the target process
 * - unsigned long addr: the address to start reading from
 * - void *vptr: a pointer to a buffer to read data into
 * - int len: the amount of data to read from the target
 *
 */


void ptrace_read(int pid, unsigned long addr, void *vptr, int len)
{
	int bytesRead = 0;
	int i = 0;
	long word = 0;
	long *ptr = (long *) vptr;
	while (bytesRead < len)
	{
		errno = 0;
		word = ptrace(PTRACE_PEEKTEXT, pid, addr + bytesRead, NULL);

		if(errno != 0){
			safe_printf("read-side mmap_lock blocked %d\n", errno);
		}

		if(word == -1)
		{
			safe_printf("ptrace(PTRACE_PEEKTEXT) failed\n");
			return;
			exit(1);
		}
		bytesRead += sizeof(word);
		ptr[i++] = word;
	}
}

/*
 * ptrace_write()
 *
 * Use ptrace() to write to the target process' address space.
 *
 * args:
 * - int pid: pid of the target process
 * - unsigned long addr: the address to start writing to
 * - void *vptr: a pointer to a buffer containing the data to be written to the
 *   target's address space
 * - int len: the amount of data to write to the target
 *
 */

void ptrace_write(int pid, unsigned long addr, void *vptr, int len)
{
	int byteCount = 0;
	long word = 0;

	while (byteCount < len)
	{
		memcpy(&word, vptr + byteCount, sizeof(word));
		word = ptrace(PTRACE_POKETEXT, pid, addr + byteCount, word);
		if(word == -1)
		{
			fprintf(stderr, "ptrace(PTRACE_POKETEXT) failed\n");
			exit(1);
		}
		byteCount += sizeof(word);
	}
}