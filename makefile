##################
# Make for C/C++ #
# v1.6           #
##################

OUTPUT_FILE := main
ATTR_ALL := -MMD -Wall
ATTR_GPP := -O3 -std=c++14
ATTR_OUT :=
ATTR_GCC :=

LIB_FILES := $(shell find lib/ -name '*.a')
LIB_FOLDER := -Llib
INCLUDE_FOLDER := -Iinclude

SRCS_GPP := $(shell find src/ -name '*.cpp')
SRCS_GCC := $(shell find src/ -name '*.c')
SRCS_SHADER := $(shell find src/shaders/ -name 'shader.*')

DEPS := $(SRCS_GPP:src/%.cpp=obj/%.d) $(SRCS_GCC:src/%.c=obj/%.d)
OBJS := $(SRCS_GPP:src/%.cpp=obj/%.o) $(SRCS_GCC:src/%.c=obj/%.o)

default: build
build: create_obj_folders compile_shaders $(OUTPUT_FILE)

rebuild: clean build

RUN_MSG := Running...
run: build
	./$(OUTPUT_FILE)

clean:
	rm -rf obj
	rm -rf shaders

create_project:
	mkdir -p src;
	mkdir -p src/shaders
	mkdir -p lib;
	mkdir -p include;
	mkdir -p shaders;

create_obj_folders: #cria pastas necessarias
	@mkdir -p $(OBJS:%.o=%)
	@mkdir -p shaders
	@rm -rf $(OBJS:%.o=%)

compile_shaders: $(SRCS_SHADER:src/shaders/shader.%=shaders/%.spv)

##################################################################

$(OUTPUT_FILE): $(OBJS)
	g++ -o $@ $(OBJS) $(LIB_FOLDER) $(ATTR_OUT) $(LIB_FILES)

obj/%.o: src/%.cpp
	g++ $(ATTR_ALL) $(ATTR_GPP) $(INCLUDE_FOLDER) -c $< -o $@

obj/%.o: src/%.c
	gcc $(ATTR_ALL) $(ATTR_GCC) $(INCLUDE_FOLDER) -c $< -o $@

shaders/%.spv: src/shaders/shader.%
	glslangValidator -V $< -o $@

-include $(DEPS)