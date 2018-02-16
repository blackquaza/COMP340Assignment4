CC = gcc

dar: main.c write.c read.c
	$(CC) main.c write.c read.c -w -o dar -lm
