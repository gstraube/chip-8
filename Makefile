CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11

chip-8: chip8.c
	$(CC) $(CFLAGS) -o chip8 chip8.c -lglut -lGL
