include mk/subdir_pre.mk

$(eval $(call subdir,syscall))

# Object files for exec module
exec_objs := $(addprefix $(d), \
	decode.o evnt.o exec.o hw_rw.o ptrace.o \
	safe_lock.o safe_printf.o thread_state.o)

ALL += $(exec_objs)
EXEC_OBJS += $(exec_objs)
CLEAN += $(exec_objs)

include mk/subdir_post.mk
