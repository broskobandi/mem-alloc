# Project
PROJECT = mem_alloc
FIND_CC := $(shell command -v clang || command -v gcc)
CC := $(FIND_CC)
CFLAGS := -Wall -Wextra -Werror -Wunused-result -Wconversion
CPPFLAGS := -Iinclude -Isrc $(EXTRA_CPPFLAGS)

# Dirs
BUILD_DIR := build
SRC_DIR := src
INC_DIR := include
TEST_DIR := test
OBJ_DIR := $(BUILD_DIR)/obj
LIB_INSTALL_DIR := /usr/local/lib
INC_INSTALL_DIR := /usr/local/include
DOC_DIR := doc
EXAMPLE_DIR := example

# Files
SRC := $(wildcard $(SRC_DIR)/*.c)
INC_PRIV := $(wildcard $(SRC_DIR)/*.h)
INC := $(INC_DIR)/$(PROJECT).h
TEST_MAIN := $(TEST_DIR)/test.c
TEST_EXE := $(BUILD_DIR)/test
LIB_SO := $(BUILD_DIR)/lib$(PROJECT).so
LIB_A := $(BUILD_DIR)/lib$(PROJECT).a
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
EXAMPLE_MAIN := $(EXAMPLE_DIR)/example.c
EXAMPLE_EXE := $(BUILD_DIR)/example

# Rules
.PHONY: all test clean install uninstall doc debug example

all: CPPFLAGS := -DNDEBUG
all: $(LIB_A) $(LIB_SO)

debug: $(LIB_A) $(LIB_SO)

test: CC := bear -- $(FIND_CC)
test: $(TEST_EXE)
	./$<

example: LDFLAGS := -L/usr/local/lib -lmem_alloc
example: CFLAGS :=
example: $(EXAMPLE_EXE)
	./$<

$(EXAMPLE_EXE): $(EXAMPLE_MAIN) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR) $(DOC_DIR) compile_commands.json

install:
	cp $(LIB_A) $(LIB_INSTALL_DIR)
	cp $(LIB_SO) $(LIB_INSTALL_DIR) 
	cp $(INC) $(INC_INSTALL_DIR)
	ldconfig

uninstall:
	rm $(addprefix $(LIB_INSTALL_DIR)/, $(notdir $(LIB_A)))
	rm $(addprefix $(LIB_INSTALL_DIR)/, $(notdir $(LIB_SO)))
	rm $(addprefix $(INC_INSTALL_DIR)/, $(notdir $(INC)))

doc:
	doxygen

$(LIB_A): $(OBJ) | $(BUILD_DIR)
	ar rcs $@ $^

$(LIB_SO): $(OBJ) | $(BUILD_DIR)
	$(CC) -shared $(CFLAGS) $(CPPFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC) $(INC_PRIV) | $(OBJ_DIR)
	$(CC) -c -fPIC $(CFLAGS) $(CPPFLAGS) $< -o $@

$(TEST_EXE): $(TEST_MAIN) $(OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@

$(OBJ_DIR):
	mkdir -p $@

$(BUILD_DIR):
	mkdir -p $@
