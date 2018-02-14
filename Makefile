CC = gcc

dar: main.c
	$(CC) main.c write.c -o dar
