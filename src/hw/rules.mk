include mk/subdir_pre.mk

# Object files for hw module
hw_objs := $(addprefix $(d), \
	mem_side_channel.o scx_pci.o)

ALL += $(hw_objs)
EXEC_OBJS += $(hw_objs)
CLEAN += $(hw_objs)

include mk/subdir_post.mk
