# Compiler and flags
CXX = g++
CXXFLAGS = -fPIC -O3

INCLUDE = -I../../../include

# Source files
SRCS = $(wildcard *.cc) $(wildcard $(FUNC_SIM_DIR)/*.cpp)

# Object files
# Update the substitution pattern to handle both .cc and .cpp files
OBJS = $(SRCS:.cc=.o)
OBJS := $(OBJS:.cpp=.o)

# Shared library
LIB = liblpn_sim.so

# Default target
all: $(LIB)

# Rule to create shared library
$(LIB): $(OBJS)
	$(CXX) $(INCLUDE) -shared -o $@ $^

# Rule to compile source files into object files
%.o: %.cc
	$(CXX) $(INCLUDE) $(CXXFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(INCLUDE) $(CXXFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
