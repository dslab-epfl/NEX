include mk/subdir_pre.mk

# Object files for syscall module
syscall_objs := $(addprefix $(d), fileops.o net.o time.o)

ALL += $(syscall_objs)
EXEC_OBJS += $(syscall_objs)
CLEAN += $(syscall_objs)

include mk/subdir_post.mk
