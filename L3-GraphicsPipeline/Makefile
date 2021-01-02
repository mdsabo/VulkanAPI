#
# 'make depend' uses makedepend to automatically generate dependencies
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .o and executable files
#

# define the C compiler to use
CC := clang++
GLSL := glslangValidator

# define any compile-time flags
CFLAGS := -Wall -std=c++17 -g
LDFLAGS:= -lvulkan -lMoltenVK -lglfw3 -framework IOKit -framework Cocoa

# define any directories containing header files other than /usr/include
#
INCLUDES := -I./include

# define the C source files
SRCS := $(shell ls src/*.cpp)

SHADERS := $(shell ls shader/*)

# define the C object files
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS := $(SRCS:.cpp=.o)
SPIRV := $(SHADERS:.vert=.spv)
SPIRV := $(SPIRV:.frag=.spv)
$(info $(SPIRV))

# define the executable file
TARGET := runme

#
# The following part of the makefile is generic; it can be used to
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean run

all: $(TARGET)

$(TARGET): $(OBJS) $(SPIRV)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file)
# (see the gnu make manual section about automatic variables)
%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

%.spv: %.vert
	$(GLSL) -V $< -o $@

%.spv: %.frag
	$(GLSL) -V $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) src/*.o shader/*.spv *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^