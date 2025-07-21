include mk/subdir_pre.mk

# Object files for accvm module
accvm_objs := $(addprefix $(d), \
	acctime.o accvm.o clone.o context.o fileops.o perf.o)

ALL += $(accvm_objs)
ACCVM_OBJS += $(accvm_objs)
CLEAN += $(accvm_objs)

include mk/subdir_post.mk
