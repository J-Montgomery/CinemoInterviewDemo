CC=gcc
CFLAGS=-std=c99 -g -O3 -Wall -Werror

all: WavConverter.exe

WavConverter.exe: 
	$(CC) $(CFLAGS) WavConverter.c -lmp3lame -static -o WavConverter

clean:
	rm WavConverter 
