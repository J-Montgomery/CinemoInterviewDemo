CC=gcc
CFLAGS=-std=c99 -g -O2 -Wall -Werror -Wno-unused

all: WavConverter.exe

WavConverter.exe: 
	$(CC) $(CFLAGS) -o Wav2Mp3 filesystem_access.c WavConverter.c -lmp3lame -lpthread -lm -static 

clean:
	rm Wav2Mp3
