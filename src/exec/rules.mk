include mk/subdir_pre.mk

$(eval $(call subdir,syscall))

# Object files for exec module
exec_objs := $(addprefix $(d), \
	decode.o evnt.o exec.o ptrace.o \
	safe_lock.o safe_printf.o thread_state.o)

ifeq ($(CONFIG_LEGACY_JPEG_DSIM)$(CONFIG_LEGACY_VTA_DSIM)$(CONFIG_LEGACY_PROTOACC_DSIM),000)
exec_objs += $(addprefix $(d), hw_rw.o)
else
exec_objs += $(addprefix $(d), hw_rw_legacy.o)
endif

ALL += $(exec_objs)
EXEC_OBJS += $(exec_objs)
CLEAN += $(exec_objs)

include mk/subdir_post.mk
