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

// The following websites were used to gain an understanding on the cpio archive headers:
//
// https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.v2r3.bpxa500/fmtcpio.htm
// https://www.mkssoftware.com/docs/man4/cpio.4.asp
// https://www.mkssoftware.com/docs/man5/struct_stat.5.asp
//
// This was all accessed prior to any coding. Since one of the bonus requirements is
// to make the file able to be extracted using the GNU cpio, this means the file
// headers must be able to be interpreted using one of the support archive formats.
// After the research, I decided on the old odc format, as it used an ASCII header
// but was not overly complicated. Since the assignment specifically calls out that
// links, fifos, and specials are not needed to implement, I've taken the rdev field
// and repurposed it to track compression, renames, and deletions.

// Everything in the odc header is in octals, so this function converts the decimals
// provided by the kernel and converts them into the correct string.
//
// Referenced the following site for information:
// http://man7.org/linux/man-pages/man7/inode.7.html

void convert(int input, char *octal, int size);
int convert2(char *input, int size); // Defined in 'read.c'

int writeArch (char *archive, int index, char **files, int compress, int verbose) {

	// Open or create the archive. Quit if there's an error.
	int fd = open(archive, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	free(archive);

	if (fd == -1) return -1;

	struct stat fileinfo;
	fstat(fd, &fileinfo);

	int init = index;

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

	}

	char *conv = NULL;

	for (int i = 0; i < index; i++) {

		if (fstatat(AT_FDCWD, files[i], &fileinfo, 0) == -1) continue;

		// Creating a string to hold the concatinated header.
		char *header = malloc((76 + strlen(files[i]) + 1) * sizeof(char));
		int isDir = 0;
		strcpy(header, "070707");

		conv = malloc(7 * sizeof(char));
		convert(fileinfo.st_dev, conv, 6);
		strcat(header, conv);
		free(conv);

		conv = malloc(7 * sizeof(char));
		convert(fileinfo.st_ino, conv, 6);
		strcat(header, conv);
		free(conv);

		// Output: First three numbers are the file type. 100 for normal and
		// 040 for directories. Last three numbers are permissions.
		conv = malloc(7 * sizeof(char));
		convert(fileinfo.st_mode, conv, 6);
		strcat(header, conv);

		// Now that the octal is showing the correct format, we'll check it
		// to see if it's a directory.

		if (conv[0] == '0' && conv[1] == '4' && conv[2] == '0') {

			// It's a directory. Scan it and put all the files inside onto
			// our list of stuff to archive.
			//
			// Referred to the following for scandirat() information:
			// http://man7.com/linux/man-pages/man3/scandir.3.html
			
			isDir = 1;

			// cpio doesn't write the current dir and parent dir links into
			// the archive, so nethier shall we.
			if (strcmp(files[i], ".") != 0 && strcmp(files[i], "..") != 0) {
				
				if (verbose) {printf ("Encountered directory %s. ", files[i]);
					      printf ("Adding contents to list.\n");}
				
				//fileinfo.st_size = 0;
				struct dirent **list;
				int temp = scandirat(AT_FDCWD, files[i], &list, NULL, NULL);
				files = realloc(files, sizeof(char *) * (index + temp));

				for (int j = 0; j < temp; j++) {

					if (strcmp((*list[j]).d_name, ".") == 0 || 
					    strcmp((*list[j]).d_name, "..") == 0) {

						// More current / parent links. Skip.

						index--;
						continue;

					}

					files[j + index] = malloc(sizeof(char) *
					(strlen((*list[j]).d_name) + strlen(files[i]) + 1));

					strcpy(files[j + index], files[i]);
					strcat(files[j + index], "/");

					strcat(files[j + index], (*list[j]).d_name);

				}

				index += temp;

			} else {
				
				// Encountered a current or parent link. Skip.
				continue;

			}

		} else if (!(conv[0] == '1' && conv[1] == '0' && conv[2] == '0')) {

			// Not a regular file or a directory. Skip it.
			continue;

		}

		free(conv);

		conv = malloc(7 * sizeof(char));
		convert(fileinfo.st_uid, conv, 6);
		strcat(header, conv);
		free(conv);

		conv = malloc(7 * sizeof(char));
		convert(fileinfo.st_gid, conv, 6);
		strcat(header, conv);
		free(conv);
		
		conv = malloc(7 * sizeof(char));
		convert(fileinfo.st_nlink, conv, 6);
		strcat(header, conv);
		free(conv);

		conv = malloc(7 * sizeof(char));
		convert(fileinfo.st_rdev, conv, 6);
		strcat(header, conv);
		free(conv);
		
		conv = malloc(12 * sizeof(char));
		convert(fileinfo.st_mtime, conv, 11);
		strcat(header, conv);
		free(conv);
		
		conv = malloc(7 * sizeof(char));
		convert(strlen(files[i]) + 1, conv, 6);
		strcat(header, conv);
		free(conv);

		conv = malloc(12 * sizeof(char));
		convert(fileinfo.st_size, conv, 11);
		strcat(header, conv);
		free(conv);

		strcat(header, files[i]);
		strcat(header, "\0");

		// Write the header to the file
		write(fd, header, 76 + strlen(files[i]) + 1);
		free(header);

		if (isDir == 0) {
			
			if (verbose) printf ("Writing file %s.\n", files[i]);
			// Copy the body into the file. No compression.
			int toCopy = open(files[i], O_RDONLY);
			char *innards = malloc((fileinfo.st_size) * sizeof(char));
			read(toCopy, innards, fileinfo.st_size);
			write(fd, innards, fileinfo.st_size);

			// Clean up.
			free(innards);
			close(toCopy);

		}

		// Remove the allocated memory of recursively found files.
		if (i >= init) free(files[i]);

	}

	// Write the ending.
	write(fd, TRAILER, TR_SIZE);

	// From the extremely help and extremely persistent errors provided by cpio
	// (hehehehehe shoot me), I've found that cpio archives have a bunch of null
	// characters at the end to size them to the nearest block. Do this now.

	while (lseek(fd, 0, SEEK_CUR) % 512 != 0) {

		write(fd, "\0", 1);

	}

	close(fd);

	return 0;

}

void convert(int input, char *octal, int size) {

	for (int j = 0; j < size; j++) {

		octal[size - j - 1] = (input % 8) + 48;
		input /= 8;

	}

	octal[size] = '\0';

}
