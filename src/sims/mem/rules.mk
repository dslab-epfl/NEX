include mk/subdir_pre.mk

# Memory simulator objects (compiled directly into executable)
# Source files from perf_sim and func_sim directories
mem_perf_srcs := $(wildcard $(d)perf_sim/*.cc) 
mem_func_srcs := $(wildcard $(d)func_sim/*.cc)

# Object files
mem_objs := $(mem_perf_srcs:.cc=.o)
mem_objs += $(mem_func_srcs:.cc=.o)

# Compilation flags
$(mem_objs): CXXFLAGS += -fPIC -O3 -Isrc/sims/common/

# Add objects to executable collection
ALL += $(mem_objs)
EXEC_OBJS += $(mem_objs)
CLEAN += $(mem_objs)

include mk/subdir_post.mk
