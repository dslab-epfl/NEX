include mk/subdir_pre.mk

# LPN simulator objects (compiled directly into executable)
# Source files
lpn_srcs := $(wildcard $(d)*.cc) 

# Object files
lpn_objs := $(lpn_srcs:.cc=.o)

# Compilation flags
$(lpn_objs): CXXFLAGS += -fPIC -O3 -I$(top_srcdir)/include

# Add objects to executable collection
ALL += $(lpn_objs)
EXEC_OBJS += $(lpn_objs)
CLEAN += $(lpn_objs)

include mk/subdir_post.mk
