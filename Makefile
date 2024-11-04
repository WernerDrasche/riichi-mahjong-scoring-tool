CC=clang
CFLAGS=-O2
score: main.c
	$(CC) $(CFLAGS) $^ -o $@
