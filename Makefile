# Porject
PROJECT := mem_alloc
FIND_CC := $(shell command -v clang || command -v gcc)
CC := $(FIND_CC)
CFLAGS := -Wall -Wextra -Werror -Wunused-result -Wconversion
CPPFLAGS := -Iinclude -Isrc

# Dirs
DOC_DIR := doc
SRC_DIR := src
INC_DIR := include
TEST_DIR := test
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
LIB_INSTALL_DIR := /usr/local/lib
INC_INSTALL_DIR := /usr/local/include
EXAMPLE_DIR := example

# Files
SRC := $(wildcard $(SRC_DIR)/*.c)
INC := $(INC_DIR)/$(PROJECT).h
INC_PRIV := $(wildcard $(SRC_DIR)/*.h)
TEST_MAIN := $(TEST_DIR)/test.c
TEST_EXE := $(BUILD_DIR)/test
LIB_A := $(BUILD_DIR)/lib$(PROJECT).a
LIB_SO := $(BUILD_DIR)/lib$(PROJECT).so
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
EXAMPLE_MAIN := $(EXAMPLE_DIR)/example.c
EXAMPLE_EXE := $(BUILD_DIR)/example
COMPILE_COMMANDS := compile_commands.json

# Rules
.PHONY: all test clean install uninstall example doc

all: $(LIB_A) $(LIB_SO)

test: CC = bear -- $(FIND_CC)
test: $(TEST_EXE)
	./$<

clean:
	rm -rf $(BUILD_DIR) $(DOC_DIR) $(COMPILE_COMMANDS)

install:
	cp $(INC) $(INC_INSTALL_DIR)
	cp $(LIB_A) $(LIB_INSTALL_DIR)
	cp $(LIB_SO) $(LIB_INSTALL_DIR)
	ldconfig

uninstall:
	rm $(addprefix $(LIB_INSTALL_DIR)/, $(notdir $(LIB_A)))
	rm $(addprefix $(LIB_INSTALL_DIR)/, $(notdir $(LIB_SO)))
	rm $(addprefix $(INC_INSTALL_DIR)/, $(notdir $(INC)))

example: $(EXAMPLE_EXE)
	./$<

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

$(BUILD_DIR):
	mkdir -p $@

$(OBJ_DIR):
	mkdir -p $@

$(EXAMPLE_EXE): $(EXAMPLE_MAIN) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ -L/usr/local/lib -lmem_alloc
