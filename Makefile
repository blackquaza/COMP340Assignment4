CC = gcc

dar: main.c write.c read.c edit.c
	$(CC) main.c write.c read.c edit.c -w -o dar -lm
