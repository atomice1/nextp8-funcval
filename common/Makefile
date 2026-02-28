# Makefile for nextp8 Functional Validation Common Library
# Builds libfuncval.a for use by test programs

# Tools
CPP = m68k-elf-cpp
CC = m68k-elf-gcc
AS = m68k-elf-as
AR = m68k-elf-ar
RANLIB = m68k-elf-ranlib

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BSP_INCLUDE_DIR = ../../nextp8-bsp/include

# Flags
CPPFLAGS = -I$(INCLUDE_DIR) -I$(BSP_INCLUDE_DIR)
CFLAGS = -m68000 -O2 -Wall -Wextra -fomit-frame-pointer -nostartfiles
ASFLAGS = -m68000 --register-prefix-optional

# Source files
ASM_SOURCES = $(wildcard $(SRC_DIR)/*.S)
C_SOURCES = $(wildcard $(SRC_DIR)/*.c)

# Object files
ASM_OBJECTS = $(patsubst $(SRC_DIR)/%.S,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
C_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

# Output library
LIBRARY = libfuncval.a

# Default target
all: $(LIBRARY)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build object files from assembly (with preprocessing)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S | $(BUILD_DIR)
	$(CPP) $(CPPFLAGS) $< | $(AS) $(ASFLAGS) -o $@

# Build object files from C
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Build library
$(LIBRARY): $(OBJECTS)
	$(AR) rcs $@ $^
	$(RANLIB) $@
	@echo "Library built: $(LIBRARY)"
	@echo "Object files:"
	@echo "  $(OBJECTS)"

# Clean
clean:
	rm -rf $(BUILD_DIR) $(LIBRARY)

# Install to common location (optional)
install: $(LIBRARY)
	cp $(LIBRARY) ../$(LIBRARY)
	@echo "Installed to ../$(LIBRARY)"

.PHONY: all clean install
