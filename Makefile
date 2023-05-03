DIR_LIB:=lib
include $(DIR_LIB)/make-pal/pal.mak
DIR_SRC:=src
DIR_INCLUDE:=include
DIR_BUILD:=build
CC:=gcc

MAIN_NAME:=scrng
MAIN_SRC:=$(wildcard $(DIR_SRC)/*.c)
MAIN_OBJ:=$(MAIN_SRC:$(DIR_SRC)/%.c=$(DIR_BUILD)/$(MAIN_NAME)/%.o)
MAIN_DEP:=$(MAIN_OBJ:%.o=%.d)
MAIN_CC_FLAGS:=\
	-W \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-unused-parameter \
	-Wconversion \
	-Wshadow \
	-O2 \
	-I$(DIR_INCLUDE) \
	-I$(DIR_LIB)/scraw/include \
	-L$(DIR_LIB)/scraw/build \
	-lscraw
MAIN_LIBSCRAW_TARGET:=main

ifeq ($(OS),Windows_NT)
	MAIN_CC_FLAGS+=-lwinscard
else
	UNAME:=$(shell uname -s)
	MAIN_CC_FLAGS+=-lpcsclite
	ifeq ($(UNAME),Linux)
		MAIN_CC_FLAGS+=$(shell pkg-config --cflags libpcsclite)
	endif
	ifeq ($(UNAME),Darwin)
		MAIN_CC_FLAGS+=-framework PCSC
	endif
endif

all: main
.PHONY: all

main: $(DIR_BUILD) $(DIR_BUILD)/$(MAIN_NAME) $(DIR_BUILD)/$(MAIN_NAME).$(EXT_BIN)
.PHONY: main

# Build the executable.
$(DIR_BUILD)/$(MAIN_NAME).$(EXT_BIN): $(DIR_LIB)/scraw/build/$(LIB_PREFIX)scraw.$(EXT_LIB_STATIC) $(MAIN_OBJ)
	$(CC) $(^) -o $(@) $(MAIN_CC_FLAGS)

# Build scraw.
$(DIR_LIB)/scraw/build/$(LIB_PREFIX)scraw.$(EXT_LIB_STATIC):
	cd $(DIR_LIB)/scraw && $(MAKE) $(MAIN_LIBSCRAW_TARGET)

# Compile source files to object files.
$(DIR_BUILD)/$(MAIN_NAME)/%.o: $(DIR_SRC)/%.c
	$(CC) $(<) -o $(@) $(MAIN_CC_FLAGS) -c -MMD

# Recompile source files after a header they include changes.
-include $(MAIN_DEP)

# For creating the necessary directories.
$(DIR_BUILD) $(DIR_BUILD)/$(MAIN_NAME):
	$(call pal_mkdir,$(@))

clean:
	$(call pal_rmdir,$(DIR_BUILD))
	cd $(DIR_LIB)/scraw && $(MAKE) clean
.PHONY: clean
