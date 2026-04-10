CC ?= cc
CFLAGS = -std=c11 -Wall -Wextra -O2
procmon: procmon.c
	$(CC) $(CFLAGS) procmon.c -lncurses -o procmon
