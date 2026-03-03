# Makefile for Test Suite: I2C RTC
# Links with rtc.c from nextp8-bsp

NEXTP8_BSP = ../../../nextp8-bsp

EXTRA_OBJS = rtc.o

TIMEOUT = 2000
export TIMEOUT

include ../Makefile.common

# Append after Makefile.common sets base values
CPPFLAGS += -I$(NEXTP8_BSP)/include -I$(NEXTP8_BSP)/src
CFLAGS += -DROM
LIBS += -L$(NEXTP8_BSP)/lib -lnextp8-rom

rtc.o: $(NEXTP8_BSP)/src/rtc.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
