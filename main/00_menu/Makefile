# Makefile for nextp8 Functional Validation Test Menu
# Builds menu.c and links with all test suites from other main/ directories
# Allows interactive selection and running of individual test suites

# Tools
CPP = m68k-elf-cpp
CC = m68k-elf-gcc
AS = m68k-elf-as
OBJCOPY = m68k-elf-objcopy
OBJDUMP = m68k-elf-objdump
PYTHON = python3

# Directories
COMMON_DIR = ../../common
TOOLS_DIR = ../../tools
NEXTP8_BSP = ../../../nextp8-bsp

# Files
TARGET = 00_menu
ELF = $(TARGET).elf
BIN = $(TARGET).bin
MEM = $(TARGET).mem
LST = $(TARGET).lst
MENU_OBJ = $(TARGET).o
CRT0_OBJ = crt0.o
CRT1_OBJ = crt1.o

# Find all test.o files from numbered test suite directories (01-14 only, exclude 00)
# Note: test object files are now named after their directory (e.g., 01_version_info.o)
# Exclude 06_sdspi (has external driver deps) and 07_i2c (now inlined, can be included)
TEST_SUITE_DIRS = $(filter-out ../00_menu ../06_sdspi, $(wildcard ../0[1-9]_* ../1[0-4]_*))
TEST_OBJS = $(foreach dir, $(TEST_SUITE_DIRS), $(dir)/$(notdir $(dir)).o)

# Flags
CPPFLAGS = -I$(COMMON_DIR)/include -I$(NEXTP8_BSP)/include
CFLAGS = -m68000 -O2 -Wall -Wextra -fomit-frame-pointer -nostartfiles
ASFLAGS = -m68000 --register-prefix-optional
LDFLAGS = -nostartfiles -T $(COMMON_DIR)/nextp8_funcval.ld
LIBS = -L$(COMMON_DIR) -lfuncval

# Default target
all: $(MEM) $(LST)
	@echo "Menu test binary built: $(BIN)"

# Compile crt0.S from common directory
$(CRT0_OBJ): $(COMMON_DIR)/crt0.S
	$(CPP) $(CPPFLAGS) $< | $(AS) $(ASFLAGS) -o $@

# Compile crt1.c from common directory
$(CRT1_OBJ): $(COMMON_DIR)/crt1.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compile 00_menu.c to 00_menu.o
$(MENU_OBJ): 00_menu.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Link menu with all test suite objects (no MAIN_OBJ - menu.c provides main())
# The linker will collect all test_suite structures from .test_suite_entry section
$(ELF): $(CRT0_OBJ) $(CRT1_OBJ) $(MENU_OBJ) $(TEST_OBJS) $(COMMON_DIR)/libfuncval.a
	$(CC) $(LDFLAGS) -o $@ $(CRT0_OBJ) $(CRT1_OBJ) $(MENU_OBJ) $(TEST_OBJS) $(LIBS)

# Convert to binary
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# Generate listing
$(LST): $(ELF)
	$(OBJDUMP) -d -S $< > $@

# Convert to .mem format
$(MEM): $(BIN)
	$(PYTHON) $(TOOLS_DIR)/bin2mem.py $< $@

sim: $(MEM)
	@$(TOOLS_DIR)/test_wrapper.sh sim

# Run with model
model: $(BIN)
	@$(TOOLS_DIR)/test_wrapper.sh model

# Alias
.PHONY: test
test: model

clean:
	rm -f $(MENU_OBJ) $(CRT0_OBJ) $(CRT1_OBJ) $(ELF) $(BIN) $(MEM) $(LST)

.PHONY: all sim model test clean
