#Makefile build system for
#KiNO_Win32 by George Koskeridis (c) 2016

CC = gcc
CFLAGS = -Wall -std=c11 -O3
LINKER = -static-libgcc -lcurldll .\win32build\libxml2.dll -lwininet -lcomctl32 -lgdi32 -ldwmapi
DELCMD = del
GUI = .\win32build\KiNO_Win32.exe
GUIDBG = .\win32build\KiNO_Win32dbg.exe


DBGFLAGS = -D__DBG -g
HDRS = k_win32gui.h k_hdr.h
GUISRC = k_win32gui_main.c
OBJS = k_algorithm.o k_xmlbase.o k_win32gui_common.o k_win32gui_locale.o k_win32gui_about.o k_win32gui_settings.o k_win32gui_progbar.o


.PHONY: gui guidbg clean
gui: $(GUI)

guidbg: $(GUIDBG)

clean:
	@echo off & \
	@$(DELCMD) *.o *.exe $(GUI) $(GUIDBG) *.dll *.a & \
	@pushd locale_resources & \
	@mingw32-make.exe clean & \
	@popd & \
	@echo on

locale_dlls:
	@echo off
	pushd locale_resources & mingw32-make.exe & popd
	@echo on

$(OBJS): %.o: %.c $(HDRS)
	$(CC) -c $(CFLAGS) $< -o $@

k_res.o: k_res.rc resource.h
	windres k_res.rc $@

$(GUIDBG): $(GUISRC) $(OBJS) k_res.o $(HDRS)
	$(CC) $(CFLAGS) $(DBGFLAGS) $^ -o $@ $(LINKER)

$(GUI): $(GUISRC) $(OBJS) k_res.o $(HDRS)
	$(CC) $(CFLAGS) $^ -mwindows -o $@ $(LINKER)
