# Compiler and flags
include ../../../config.mk
CXX = g++
CXXFLAGS = -fPIC -O3 -std=c++17

INCLUDE = -I../common/ -I$(CONFIG_PROJECT_PATH)/json_lib/
# Directories
PERF_SIM_DIR = perf_sim
FUNC_SIM_DIR = func_sim

# Source files
PERF_SIM_SRCS = $(wildcard $(PERF_SIM_DIR)/*.cc) $(wildcard $(PERF_SIM_DIR)/*.cpp)
FUNC_SIM_SRCS = $(wildcard $(FUNC_SIM_DIR)/*.cc) $(wildcard $(FUNC_SIM_DIR)/*.cpp)
SRCS = $(PERF_SIM_SRCS) $(FUNC_SIM_SRCS)

# Object files
# Update the substitution pattern to handle both .cc and .cpp files
OBJS = $(SRCS:.cc=.o)
OBJS := $(OBJS:.cpp=.o)

# Shared library
LIB = libprotoacc_sim.so

# Default target
all: $(LIB)

# Rule to create shared library
$(LIB): $(OBJS)
	$(CXX) -shared -o $@ $^

# Rule to compile source files into object files
%.o: %.cc
	$(CXX) $(INCLUDE) $(CXXFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(INCLUDE) $(CXXFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
