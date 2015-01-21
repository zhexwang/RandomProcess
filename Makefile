SHELL := /bin/bash
BIN := random

BUILD_DIR := build
SRC_DIR := main memory
DISASM_DIR := disasm

DISASM_INCLUDE := -I./include -I./include/beaengine
INCLUDE := -I./include 

OPTIMIZE := -O0 -g
CFLAGS := $(OPTIMIZE) -Wall 
DISASM_CFLAGS := $(OPTIMIZE) -fPIC -Wall -pedantic -ansi -pipe -fno-common -fshort-enums -W -Wextra -Wconversion -Wno-long-long \
	-Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings

LDFLAGS := 
LIBS := -ldl -lrt

CXX := g++ 
CC := gcc

#########################################
### DO NOT MODIFY THE FOLLOWING LINES ###          
#########################################

SRC := $(foreach d,${SRC_DIR},$(wildcard ${d}/*.cpp))
SRC += $(foreach d,${SRC_DIR},$(wildcard ${d}/*.c))
DISASM_SRC = $(DISASM_DIR)/BeaEngine.c

OBJ := $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.cpp,%.o,$(wildcard ${d}/*.cpp)))))
OBJ += $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.c,%.o,$(wildcard ${d}/*.c)))))
DISASM_OBJ = $(DISASM_DIR)/disassember.o

DEP := $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.cpp,%.d,$(wildcard ${d}/*.cpp)))))
DEP += $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.c,%.d,$(wildcard ${d}/*.c)))))
DISASM_DEP := $(BUILD_DIR)/BeaEngine.d

.PHONY: all clean 

vpath %.cpp $(SRC_DIR)
vpath %.c $(SRC_DIR)

all: $(DISASM_OBJ) $(OBJ) 
	@if \
	$(CXX) $(DISASM_OBJ) $(OBJ) $(LDFLAGS) -o $(BIN) $(LIBS);\
	then echo -e "[\e[32;1mLINK\e[m] \e[33m$(DISASM_OBJ) $(OBJ)\e[m \e[36m->\e[m \e[32;1m$(BIN)\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$(DISASM_OBJ) $(OBJ)\e[m \e[36m->\e[m \e[32;1m$(BIN)\e[m"; exit -1; fi;
clean:
	@echo -e "[\e[32mCLEAN\e[m] \e[33m$(BIN) $(BUILD_DIR)/\e[m"
	@rm -rf $(BIN) build  

sinclude $(DISASM_DEP)
sinclude $(DEP)

$(BUILD_DIR)/%.d: %.cpp Makefile
	@mkdir -p $(BUILD_DIR);
	@if \
	$(CXX) ${INCLUDE} -MM $< > $@ && sed -i '1s/^/$(BUILD_DIR)\//g' $@; \
	then echo -e "[\e[32mCXX \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

$(BUILD_DIR)/%.d: %.c Makefile
	@mkdir -p $(BUILD_DIR);
	@if \
	$(CC) ${INCLUDE} -MM $< > $@ && sed -i '1s/^/$(BUILD_DIR)\//g' $@;\
	then echo -e "[\e[32mCC  \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

$(DISASM_DEP): $(DISASM_SRC) Makefile
	@mkdir -p $(BUILD_DIR);
	@if \
	$(CC) ${INCLUDE} -MM $< > $@ && sed -i '1s/^/$(BUILD_DIR)\//g' $@;\
	then echo -e "[\e[32mCC  \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

$(BUILD_DIR)/%.o: %.cpp Makefile
	@if \
	$(CXX) ${CFLAGS} ${INCLUDE} -c $< -o $@; \
	then echo -e "[\e[32mCXX \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

$(BUILD_DIR)/%.o: %.c Makefile
	@if \
	$(CC) ${CFLAGS} ${INCLUDE} -c $< -o $@; \
	then echo -e "[\e[32mCC  \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

$(DISASM_OBJ): $(DISASM_SRC) Makefile
	@if \
	$(CC) ${DISASM_CFLAGS} ${DISASM_INCLUDE} -c $< -o $@; \
	then echo -e "[\e[32mCC  \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;
