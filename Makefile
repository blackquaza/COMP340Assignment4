CC = gcc

dar: main.c write.c
	$(CC) main.c write.c -o dar

ignore: main.c write.c
	$(CC) main.c write.c -w -o dar
