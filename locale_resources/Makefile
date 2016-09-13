#Makefile build system for
#KiNO_Win32 by George Koskeridis (c) 2016

CC = gcc
CFLAGS = -Wall -std=c11 -O3
DELCMD = del

LOCALE_RCS = $(wildcard *.rc)
LOCALE_OBJS = $(patsubst %.rc,%.o,$(LOCALE_RCS))
LOCALE_DLLS = $(patsubst %.o,%.dll,$(LOCALE_OBJS))

all: $(LOCALE_DLLS)

$(LOCALE_OBJS): %.o: %.rc
	windres $< -o $@

$(LOCALE_DLLS): %.dll: %.o
	$(CC) -shared $< -o ../win32build/lang/$@


.PHONY: clean
clean:
	@echo off
	@$(DELCMD) *.o *.dll ..\win32build\lang\*.dll
	@echo on
