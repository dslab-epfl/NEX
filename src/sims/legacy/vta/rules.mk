include mk/subdir_pre.mk

# Legacy VTA shared library
vta_legacy_srcs := $(wildcard $(d)perf_sim/*.cc) $(wildcard $(d)perf_sim/*.cpp) \
                   $(wildcard $(d)func_sim/*.cc) $(wildcard $(d)func_sim/*.cpp)

# Object files
vta_legacy_objs := $(vta_legacy_srcs:.cc=.o)
vta_legacy_objs := $(vta_legacy_objs:.cpp=.o)

# Shared library target
vta_legacy_lib := $(d)libvta_sim.so

# Compilation flags
$(vta_legacy_objs): CXXFLAGS += -fPIC -O3 -std=c++17 -I$(top_srcdir)/src/sims/common/ -I$(top_srcdir)/include

# Rule to create shared library
$(vta_legacy_lib): $(vta_legacy_objs)
	$(CXX) -shared -o $@ $^

# Add to build targets
ALL += $(vta_legacy_lib)
LEGACY_LIB_TARGETS += $(vta_legacy_lib)
LEGACY_LIBS += -L$(CURDIR)/$(d) -lvta_sim
# Add RPATH for this legacy library
LEGACY_RPATH += -Wl,-rpath,$(CURDIR)/$(d)
CLEAN += $(vta_legacy_objs) $(vta_legacy_lib)

include mk/subdir_post.mk
