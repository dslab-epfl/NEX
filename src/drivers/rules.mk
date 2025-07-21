include mk/subdir_pre.mk

# Object files for drivers module
drivers_objs := $(addprefix $(d), \
	driver_common.o jpegdecoder_driver.o)

ALL += $(drivers_objs)
EXEC_OBJS += $(drivers_objs)
CLEAN += $(drivers_objs)

include mk/subdir_post.mk
