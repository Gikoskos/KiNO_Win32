# KiNO_Win32
WinAPI application to download KINO lotteries and get winning prices for a fixed number of tickets.

At the moment the algorithm works for up to 8 tickets with 10 numbers.

You fill out 8 custom tickets or leave it with the default 8, and the program downloads the selected KINO lotteries and finds 
how much your ticket costs for each lottery.

A user can select which lotteries to download in 3 different ways. By selecting the dates of the lotteries, by setting a first lottery number and a last lottery number 
(so that the program downloads the lotteries from the first up to the last lottery) or by getting the latest x lotteries played.

## Building

Supported compilers: mingw-w64

Run the makefile to compile the GUI:

    mingw32-make.exe

You need to have libxml.dll on the win32build folder with this makefile. If you have it installed somewhere else
and you're linking with a loader library (.lib, .a) change the makefile accordingly.

To compile the language DLLs run:

    mingw32-make.exe locale_dlls

## Libs used

[libcurl](http://curl.haxx.se/libcurl/)

[libxml2](http://xmlsoft.org/)


## License

see LICENSE for KiNO_Win32

see win32build/LICENSES for the libraries

KiNO_Win32 (C) 2016 <georgekoskerid@outlook.com>
