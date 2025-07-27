include mk/subdir_pre.mk

# Legacy Protoacc shared library
protoacc_legacy_srcs := $(wildcard $(d)perf_sim/*.cc) $(wildcard $(d)perf_sim/*.cpp) \
                        $(wildcard $(d)func_sim/*.cc) $(wildcard $(d)func_sim/*.cpp)

# Object files
protoacc_legacy_objs := $(protoacc_legacy_srcs:.cc=.o)
protoacc_legacy_objs := $(protoacc_legacy_objs:.cpp=.o)

# Shared library target
protoacc_legacy_lib := $(d)libprotoacc_sim.so

# Compilation flags (note the special json_lib include)
$(protoacc_legacy_objs): CXXFLAGS += -fPIC -O3 -std=c++17 -I$(top_srcdir)/src/sims/common/ -I$(top_srcdir)/include -I$(CONFIG_PROJECT_PATH)/json_lib/

# Rule to create shared library
$(protoacc_legacy_lib): $(protoacc_legacy_objs)
	$(CXX) -shared -o $@ $^

# Add to build targets
ALL += $(protoacc_legacy_lib)
LEGACY_LIB_TARGETS += $(protoacc_legacy_lib)
LEGACY_LIBS += -L$(CURDIR)/$(d) -lprotoacc_sim
# Add RPATH for this legacy library
LEGACY_RPATH += -Wl,-rpath,$(CURDIR)/$(d)
CLEAN += $(protoacc_legacy_objs) $(protoacc_legacy_lib)


include mk/subdir_post.mk
