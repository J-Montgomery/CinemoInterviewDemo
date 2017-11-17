# Cinemo_demo

TASK:
Write a C/C++ commandline application that encodes a set of WAV files to MP3

Requirements:

(1) application is called with pathname as argument, e.g. 
<applicationname> F:\MyWavCollection all WAV-files contained directly in that folder are to be encoded to MP3

(2) use all available CPU cores for the encoding process in an efficient way by utilizing multi-threading

(3) statically link to lame encoder library

(4) application should be compilable and runnable on Windows and Linux

(5) the resulting MP3 files are to be placed within the same directory as the source WAV files, the filename extension should be changed appropriately to .MP3

(6) non-WAV files in the given folder shall be ignored

(7) multithreading shall be implemented by means of using Posix Threads (there exist implementations for Windows)

(8) the Boost library shall not be used

(9) the LAME encoder should be used with reasonable standard settings (e.g. quality based encoding with quality level "good")

***
Libraries used: dirent.h, pthreads.h, getopt.h, libmp3lame

Compilation notes on Windows:
3 of the specific libraries used (dirent.h, pthreads.h, and getopt.h) are typically considered standard linux libraries and are not part of a default Windows SDK. Simple, windows-compatible ports of these libraries have been included in the project. libmp3lame's standard source release only compiles on x86 in VS2017 and so the VS solution has been limited to compiling in x86 mode for static linking. A .lib file compiled with /MT is provided within \lib\lame. The code should also compile in MinGW using the included makefile, but this has not been tested.

Compilation notes for Linux:
All libraries used are standard on Linux distributions except for libmp3lame. A makefile has been provided.

Usage notes:

Although the program works according to the specifications outlined above, it also contains a number of additional command line options that provide useful functionality, including a --max-cores flag to limit the number of cores utilized and a --quality flag that defaults as required.