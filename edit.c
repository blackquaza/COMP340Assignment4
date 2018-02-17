#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>

#define _POSIX_C_SOURCE
#define _GNU_SOURCE

// Defining the trailer string and size of the archive.
#define TRAILER "0707070000000000000000000000000000000000010000000000000000000001300000000000TRAILER!!!\0"
#define TR_SIZE 86

// Defined in 'write.c' and 'read.c' accordingly.
void convert(int input, char *octal, int size);
int convert2(char *input, int size);

// Most of this was copied from the write function. However, they are different enough
// to warrant separate functions. Mainly because one actually writes from files and
// this one 'makes them up'.
int editArch (char *archive, int index, char **files, int rename, int verbose) {

	if (rename && index % 2 == 1) {

		printf ("Incorrent number of arguments. Use the --help ");
		printf ("option for usage information.\n");
		return -1;

	}

	// Open or create the archive. Quit if there's an error.
	int fd = open(archive, O_RDWR);
	free(archive);

	if (fd == -1) return -1;

	struct stat fileinfo;
	fstat(fd, &fileinfo);

	if (fileinfo.st_size > TR_SIZE - 1) {

		// Archive already exists. Set the archive to the end. Because the archive is
		// filled with NULL characters, we instead have to loop through the items in
		// the archive to get the end.

		char *buf = NULL;
		int i = 0;
		while (1) {

			int j = 0;
			lseek(fd, i + 59, SEEK_SET);
			buf = malloc(6 * sizeof(char));
			read(fd, buf, 6);
			j = convert2(buf, 6);
			free(buf);
			lseek(fd, i + 76, SEEK_SET);
			buf = malloc(j * sizeof(char));
			read(fd, buf, j);

			if (strcmp(buf, "TRAILER!!!\0") == 0) {
				
				// Found the end. Set the writing point to be just
				// before the trailer. It'll be overwritten, but
				// we'll rewrite it later.
				j = lseek(fd, i, SEEK_SET);
				free(buf);
				break;

			}

			free(buf);
			lseek(fd, i + 65, SEEK_SET);
			buf = malloc(11 * sizeof(char));
			read(fd, buf, 11);
			j += convert2(buf, 11);
			free(buf);
			i += j + 76;

		}

	} else return -1;

	char *conv = NULL;
	char *header = NULL;
	char *data = NULL;

	for (int i = 0; i < index; i++) {

		// Creating a string to hold the concatinated header.
		header = malloc(86 * sizeof(char));
		strcpy(header, "07070700000000000000000000000000000000000100000000000000000000013");
		
		int length;
		if (rename) {
			length = strlen(files[i]) + 2 + strlen(files[i+1]);
		} else {
			length = strlen(files[i]);
		}

		conv = malloc(12 * sizeof(char));
		convert(length, conv, 11);
		strcat(header, conv);
		free(conv);

		if (rename) {
			strcat(header, "RENAME!!!!");
		} else {
			strcat(header, "DELETE!!!!");
		}

		strcat(header, "\0");
		write(fd, header, 87);
		free(header);

		data = malloc(length * sizeof(char));
		strcpy(data, files[i]);
		
		if (rename) {
			strcat(data, "//");
			strcat(data, files[++i]);
		}

		write(fd, data, length);
		free(data);

	}

	// Write the ending.
	write(fd, TRAILER, TR_SIZE);

	while (lseek(fd, 0, SEEK_CUR) % 512 != 0) {

		write(fd, "\0", 1);

	}

	close(fd);

	return 0;

}
