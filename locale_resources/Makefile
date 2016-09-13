#Makefile build system for
#KiNO_Win32 by George Koskeridis (c) 2016

CC = gcc
CFLAGS = -Wall -std=c11 -O3
DELCMD = del

LOCALE_OBJS = EnglishLang.o GreekLang.o
LOCALE_DLLS = EnglishLang.dll GreekLang.dll

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
