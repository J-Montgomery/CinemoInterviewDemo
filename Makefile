CC=gcc
CFLAGS=-std=c99 -g -O2 -Wall -Werror

all: WavConverter.exe

WavConverter.exe: 
	$(CC) $(CFLAGS) -o Release/WavConverter WavConverter/filesystem_access.c WavConverter/WavConverter.c -lmp3lame -lpthread -lm -static 

clean:
	rm Release/WaveConverter
