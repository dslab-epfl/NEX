#define _GNU_SOURCE
#include <accvm/context.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdarg.h>

#define JAIL_BREAK_NO_HALT 0x7000
#define JAIL_NO_HALT 0x8000

extern void update_ctrl_reg(uint64_t value);

extern int ebs_is_on();

// // mmap wrapper
// void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
//     if(ebs_is_on()){
//         printf("mmap: EBS is on, updating state for jail break\n");
//         update_ctrl_reg(JAIL_BREAK_NO_HALT);
//         void* ret = orig_mmap(addr, length, prot, flags, fd, offset);
//         update_ctrl_reg(JAIL_NO_HALT);
//         printf("mmap: Exit jail break\n");
//         return ret;
//     }else{
//         return orig_mmap(addr, length, prot, flags, fd, offset);
//     }
// }

// // munmap wrapper
// int munmap(void *addr, size_t length) {
//     if(ebs_is_on()){
//         printf("munmap: EBS is on, updating state for jail break\n");
//         update_ctrl_reg(JAIL_BREAK_NO_HALT);
//         int ret = orig_munmap(addr, length);
//         update_ctrl_reg(JAIL_NO_HALT);
//         printf("munmap: Exit jail break\n");
//         return ret;
//     }else{
//         return orig_munmap(addr, length);
//     }
// }

// // mprotect wrapper
// int mprotect(void *addr, size_t len, int prot) {
//     if(ebs_is_on()){
//         printf("mprotect: EBS is on, updating state for jail break\n");
//         update_ctrl_reg(JAIL_BREAK_NO_HALT);
//         int ret = orig_mprotect(addr, len, prot);
//         update_ctrl_reg(JAIL_NO_HALT);
//         printf("mprotect: Exit jail break\n");
//         return ret;
//     }else{
//         return orig_mprotect(addr, len, prot);
//     }
// }

// // mremap wrapper
// void *mremap(void *old_address, size_t old_size, size_t new_size, int flags, ...) {
//     if(ebs_is_on()){
//         printf("mremap: EBS is on, updating state for jail break\n");
//         update_ctrl_reg(JAIL_BREAK_NO_HALT);
        
//         va_list args;
//         va_start(args, flags);
//         void *new_address = NULL;
//         void *ret;
        
//         if (flags & MREMAP_FIXED) {
//             new_address = va_arg(args, void *);
//             ret = orig_mremap(old_address, old_size, new_size, flags, new_address);
//         } else {
//             ret = orig_mremap(old_address, old_size, new_size, flags);
//         }
        
//         va_end(args);
//         update_ctrl_reg(JAIL_NO_HALT);
//         printf("mremap: Exit jail break\n");
//         return ret;
//     }else{
//         va_list args;
//         va_start(args, flags);
//         void *new_address = NULL;
        
//         if (flags & MREMAP_FIXED) {
//             new_address = va_arg(args, void *);
//         }
        
//         va_end(args);
        
//         if (flags & MREMAP_FIXED) {
//             return orig_mremap(old_address, old_size, new_size, flags, new_address);
//         } else {
//             return orig_mremap(old_address, old_size, new_size, flags);
//         }
//     }
// }

// // brk wrapper
// int brk(void *addr) {
//     if(ebs_is_on()){
//         printf("brk: EBS is on, updating state for jail break\n");
//         update_ctrl_reg(JAIL_BREAK_NO_HALT);
//         int ret = orig_brk(addr);
//         update_ctrl_reg(JAIL_NO_HALT);
//         printf("brk: Exit jail break\n");
//         return ret;
//     }else{
//         return orig_brk(addr);
//     }
// }

// // sbrk wrapper
// void *sbrk(intptr_t increment) {
//     if(ebs_is_on()){
//         printf("sbrk: EBS is on, updating state for jail break\n");
//         update_ctrl_reg(JAIL_BREAK_NO_HALT);
//         void *ret = orig_sbrk(increment);
//         update_ctrl_reg(JAIL_NO_HALT);
//         printf("sbrk: Exit jail break\n");
//         return ret;
//     }else{
//         return orig_sbrk(increment);
//     }
// }

// // shmat wrapper
// void *shmat(int shmid, const void *shmaddr, int shmflg) {
//     if(ebs_is_on()){
//         printf("shmat: EBS is on, updating state for jail break\n");
//         update_ctrl_reg(JAIL_BREAK_NO_HALT);
//         void *ret = orig_shmat(shmid, shmaddr, shmflg);
//         update_ctrl_reg(JAIL_NO_HALT);
//         printf("shmat: Exit jail break\n");
//         return ret;
//     }else{
//         return orig_shmat(shmid, shmaddr, shmflg);
//     }
// }

// // shmdt wrapper
// int shmdt(const void *shmaddr) {
//     if(ebs_is_on()){
//         printf("shmdt: EBS is on, updating state for jail break\n");
//         update_ctrl_reg(JAIL_BREAK_NO_HALT);
//         int ret = orig_shmdt(shmaddr);
//         update_ctrl_reg(JAIL_NO_HALT);
//         printf("shmdt: Exit jail break\n");
//         return ret;
//     }else{
//         return orig_shmdt(shmaddr);
//     }
// }

// // dlopen wrapper
// void *dlopen(const char *filename, int flags) {
//     if(ebs_is_on()){
//         printf("dlopen: EBS is on, updating state for jail break\n");
//         update_ctrl_reg(JAIL_BREAK_NO_HALT);
//         void *ret = orig_dlopen(filename, flags);
//         update_ctrl_reg(JAIL_NO_HALT);
//         printf("dlopen: Exit jail break\n");
//         return ret;
//     }else{
//         return orig_dlopen(filename, flags);
//     }
// }

// // dlclose wrapper
// int dlclose(void *handle) {
//     if(ebs_is_on()){
//         printf("dlclose: EBS is on, updating state for jail break\n");
//         update_ctrl_reg(JAIL_BREAK_NO_HALT);
//         int ret = orig_dlclose(handle);
//         update_ctrl_reg(JAIL_NO_HALT);
//         printf("dlclose: Exit jail break\n");
//         return ret;
//     }else{
//         return orig_dlclose(handle);
//     }
// }
