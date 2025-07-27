include mk/subdir_pre.mk

# Legacy JPEG decoder shared library
jpeg_legacy_srcs := $(wildcard $(d)perf_sim/*.cc) $(wildcard $(d)perf_sim/*.cpp) \
                    $(wildcard $(d)func_sim/*.cc) $(wildcard $(d)func_sim/*.cpp)

# Object files
jpeg_legacy_objs := $(jpeg_legacy_srcs:.cc=.o)
jpeg_legacy_objs := $(jpeg_legacy_objs:.cpp=.o)

# Shared library target
jpeg_legacy_lib := $(d)libjpeg_sim.so

# Compilation flags
$(jpeg_legacy_objs): CXXFLAGS += -fPIC -O3 -std=c++17 -I$(top_srcdir)/src/sims/common/ -I$(top_srcdir)/include

# Rule to create shared library
$(jpeg_legacy_lib): $(jpeg_legacy_objs)
	$(CXX) -shared -o $@ $^

# Add to build targets
ALL += $(jpeg_legacy_lib)
LEGACY_LIB_TARGETS += $(jpeg_legacy_lib)
LEGACY_LIBS += -L$(CURDIR)/$(d) -ljpeg_sim
# Add RPATH for this legacy library
LEGACY_RPATH += -Wl,-rpath,$(CURDIR)/$(d)
CLEAN += $(jpeg_legacy_objs) $(jpeg_legacy_lib)

include mk/subdir_post.mk
