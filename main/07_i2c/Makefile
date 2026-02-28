# Makefile for Test Suite: I2C RTC
# Links with rtc.c from nextp8-bsp

# Include common settings but override linking
COMMON_DIR = ../../common
TOOLS_DIR = ../../tools
NEXTP8_BSP = ../../../nextp8-bsp

TARGET = 07_i2c
OBJ = $(TARGET).o
CRT0_OBJ = crt0.o
MAIN_OBJ = main.o
CRT1_OBJ = crt1.o
ELF = $(TARGET).elf
BIN = $(TARGET).bin
MEM = $(TARGET).mem
LST = $(TARGET).lst



# Tools
CPP = m68k-elf-cpp
CC = m68k-elf-gcc
AS = m68k-elf-as
OBJCOPY = m68k-elf-objcopy
OBJDUMP = m68k-elf-objdump
PYTHON = python3

# Flags
CPPFLAGS = -I$(COMMON_DIR)/include -I$(NEXTP8_BSP)/include -I$(NEXTP8_BSP)/src
CFLAGS = -m68000 -O2 -Wall -Wextra -fomit-frame-pointer -nostartfiles
ASFLAGS = -m68000 --register-prefix-optional
LDFLAGS = -nostartfiles -T $(COMMON_DIR)/nextp8_funcval.ld
LIBS = -L$(COMMON_DIR) -lfuncval -L$(NEXTP8_BSP)/lib -lnextp8-ram

# Default target
all: $(MEM) $(LST)

# Compile crt0.S from common directory
$(CRT0_OBJ): $(COMMON_DIR)/crt0.S
	$(CPP) $(CPPFLAGS) $< | $(AS) $(ASFLAGS) -o $@

# Compile crt1.c from common directory
$(CRT1_OBJ): $(COMMON_DIR)/crt1.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compile main.c from common directory
$(MAIN_OBJ): $(COMMON_DIR)/main.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compile test source
$(OBJ): $(TARGET).c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Link with all objects
$(ELF): $(CRT0_OBJ) $(CRT1_OBJ) $(MAIN_OBJ) $(OBJ) $(COMMON_DIR)/libfuncval.a
	$(CC) $(LDFLAGS) -o $@ $(CRT0_OBJ) $(CRT1_OBJ) $(MAIN_OBJ) $(OBJ) $(LIBS)

# Convert to binary
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# Generate listing
$(LST): $(ELF)
	$(OBJDUMP) -d -S $< > $@

# Convert to .mem format
$(MEM): $(BIN)
	$(PYTHON) $(TOOLS_DIR)/bin2mem.py $< $@

# Clean
clean:
	rm -f $(CRT0_OBJ) $(CRT1_OBJ) $(MAIN_OBJ) $(OBJ) $(ELF) $(BIN) $(MEM) $(LST)

.PHONY: all clean
