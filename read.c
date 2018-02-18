#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <math.h>

#define _POSIX_C_SOURCE
#define _GNU_SOURCE

// Refer to 'write.c' for the list of websites accessed while creating the project.

// Converts octal back into decimal.
int convert2(char *input, int size);

int readArch(char *archive, int readonly, int verbose) {

	if (readonly) printf ("The archive contains the following:\n");

	// Open the archive file. Exit if there's an error.
	int fd = open(archive, O_RDONLY);
	if (fd < 0) return fd;
	free(archive);

	// index represents the beginning of a header.
	int index = 0;
	char *input = NULL;

	while (1) {

		// Get the name length, which is characters 59 to 64 (6 total).
		lseek(fd, index + 59, SEEK_SET);
		input = malloc(6 * sizeof(char));
		read(fd, input, 6);
		int namelength = convert2(input, 6);
		free(input);

		// Now read the file name itself.
		char *filename = malloc(namelength * sizeof(char));
		lseek(fd, index + 76, SEEK_SET);
		read(fd, filename, namelength);

		// If the filename is 'TRAILER!!!', then we've reached the end
		// of the archive. Break the loop.
		if (strcmp(filename, "TRAILER!!!\0") == 0) break;

		int flag = 0;

		if (strcmp(filename, "RENAME!!!!\0") == 0) {
		       
			flag = 1;

		} else if (strcmp(filename, "DELETE!!!!\0") == 0) {

			flag = 2;

		}

		if (readonly) {

			// This is the listing section. Fire off the name,
			// move the index, and move on to the next one.
			if (!flag) {

				printf ("%s\n", filename);

			}

			lseek(fd, index + 65, SEEK_SET);
			input = malloc(11 * sizeof(char));
			read(fd, input, 11);
			int temp = convert2(input, 11);
			index += 76 + namelength + temp;
			free(input);
			continue;

		}

		int filesize = 0;

		// If its either rename or delete. The process is similar.
		if (flag) {

			lseek(fd, index + 65, SEEK_SET);
			input = malloc(11 * sizeof(char));
			read(fd, input, 11);
			filesize = convert2(input, 11);
			free(input);
			lseek(fd, index + 76 + namelength, SEEK_SET);
			char *file1 = malloc((filesize + 1) * sizeof(char));
			char *file2 = NULL;
			read(fd, file1, filesize);
			strcat(file1, "\0");

			if (flag == 1) {

				int pos = 0;

				while (file1[pos] != '>' || file1[pos+1] != '<') {

					pos++;
					if (pos == filesize - 1) return -1;

				}

				free(file1);
				file1 = malloc((pos + 1) * sizeof(char));
				lseek(fd, index + 76 + namelength, SEEK_SET);
				read(fd, file1, pos);
				strcat(file1, "\0");
				lseek(fd, 2, SEEK_CUR);
				file2 = malloc((filesize - pos - 1) * sizeof(char));
				read(fd, file2, filesize - pos - 2);
				strcat(file2, "\0");

				if (verbose) printf ("Renaming %s to %s.\n", file1, file2);
				rename(file1, file2);

			} else {

				if (verbose) printf ("Deleting %s.\n", file1);
				remove(file1);

			}

			free(file1);
			free(file2);
			free(filename);
			index += 76 + namelength + filesize;
			continue;

		}

		// Collect the file type and permissions information.
		int isDir = 0;
		lseek(fd, index + 18, SEEK_SET);
		input = malloc(6 * sizeof(char));
		read(fd, input, 6);
		if (input[0] == '0' && input[1] == '4' && input[2] == '0') isDir = 1;
		mode_t mode = convert2(input, 6);
		free(input);

		// I don't care if this clobbers any existing files.
		
		int writing = 0;

		if (isDir == 1) {

			if (verbose) printf ("Extracting directory %s.\n", filename);
			writing = mkdir(filename, mode);

		} else {

			if (verbose) printf ("Extracting file %s.\n", filename);
			writing = creat(filename, mode);

			// Get the size of the file.
			lseek(fd, index + 65, SEEK_SET);
			input = malloc(11 * sizeof(char));
			read(fd, input, 11);
			filesize = convert2(input, 11);
			free(input);

			if (filesize != 0) {

				// If the file has contents, write the contents to the file.
				lseek(fd, index + 76 + namelength, SEEK_SET);
				char *innards = malloc(filesize * sizeof(char));
				read(fd, innards, filesize);
				write(writing, innards, filesize);
				free(innards);

			}

		}

		index += 76 + namelength + filesize;

		// Cleanup.
		close(writing);
		free(filename);

	}

	free(input);


}

int convert2(char *input, int size) {

	int output = 0;

	for (int i = 0; i < size; i++) {

		output += (input[i] - 48) * pow(8, size - i - 1);

	}

	return output;

}
