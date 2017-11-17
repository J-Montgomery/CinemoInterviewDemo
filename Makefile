CC=gcc
CFLAGS=-std=c99 -g -O3 -Wall -Werror

all: WavConverter.exe

WavConverter.exe: 
	$(CC) $(CFLAGS) WavConverter/WavConverter.c -lmp3lame -lpthread -static -o Release/WavConverter

clean:
	rm Release/WaveConverter
