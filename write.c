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

int writeArch (char *archive, int index, char **files, int compress, int verbose) {

	// Open or create the archive. Quit if there's an error.
	int fd = open(archive, O_RDWR, O_CREAT);
	free(archive);

	if (fd == -1) return -1;

	struct stat fileinfo;
	fstat(fd, &fileinfo);

	if (fileinfo.st_size <= TR_SIZE) {

		// Archive is newly created. Set the offset to the beginning.
		lseek(fd, SEEK_SET, 0);

		// Write the end of the file.
		write(fd, TRAILER, TR_SIZE);

		fileinfo.st_size += TR_SIZE;

	}
	
	// The archive is now guaranteed to have an end. Start writing to just before it.
	// THIS IS OVERWRITTEN BY EVERYTHING ELSE. We'll re-write the ending later.
	int t = lseek(fd, SEEK_SET, fileinfo.st_size - TR_SIZE);

	for (int i = 0; i < index; i++) {

		fstatat(AT_FDCWD, files[i], &fileinfo, 0);

		// Creating a string to hold the concatinated header.
		char *header = malloc((76 + strlen(files[i])) * sizeof(char));
		char conv1[6];
		char conv2[11];
		int isDir = 0;
		strcpy(header, "070707");

		convert(fileinfo.st_dev, conv1, 6);
		strcat(header, conv1);
		convert(fileinfo.st_ino, conv1, 6);
		strcat(header, conv1);

		// Output: First three numbers are the file type. 100 for normal and
		// 040 for directories. Last three numbers are permissions.
		
		convert(fileinfo.st_mode, conv1, 6);
		strcat(header, conv1);

		// Now that the octal is showing the correct format, we'll check it
		// to see if it's a directory.

		if (conv1[0] == '0' && conv1[1] == '4' && conv1[2] == '0') {

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
					(strlen((*list[j]).d_name) + strlen(files[i] + 1)));

					strcpy(files[j + index], files[i]);
					strcat(files[j + index], "/");

					strcat(files[j + index], (*(list[j])).d_name);

				}

				index += temp;

			} else {
				
				// Encountered a current or parent link. Skip.
				continue;

			}

		} else if (!(conv1[0] == '1' && conv1[1] == '0' && conv1[2] == '0')) {

			// Not a regular file or a directory. Skip it.
			continue;

		}

		convert(fileinfo.st_uid, conv1, 6);
		strcat(header, conv1);
		convert(fileinfo.st_gid, conv1, 6);
		strcat(header, conv1);
		convert(fileinfo.st_nlink, conv1, 6);
		strcat(header, conv1);
		convert(fileinfo.st_rdev, conv1, 6);
		strcat(header, conv1);
		convert(fileinfo.st_mtime, conv2, 11);
		strcat(header, conv2);
		convert(strlen(files[i]) + 1, conv1, 6);
		strcat(header, conv1);
		convert(fileinfo.st_size, conv2, 11);
		strcat(header, conv2);

		strcat(header, files[i]);
		strcat(header, "\0");

		// Write the header to the file
		write(fd, header, 76 + strlen(files[i]) + 1);

		if (isDir == 0) {
			
			if (verbose) printf ("Writing file %s.\n", files[i]);
			// Copy the body into the file. No compression.
			int toCopy = open(files[i], O_RDONLY);
			char *innards = malloc((fileinfo.st_size + 2) * sizeof(char));
			read(toCopy, innards, fileinfo.st_size + 1);
			strcat(innards, "\0");
			write(fd, innards, fileinfo.st_size);

			// Clean up.
			free(innards);
			close(toCopy);

		}

	}

	// Rewrite the ending, cause we deleted it. Trollolol.
	write(fd, TRAILER, TR_SIZE);

	// From the extremely help and extremely persistent errors provided by cpio
	// (hehehehehe shoot me), I've found that cpio archives have a bunch of null
	// characters at the end to size them to the nearest block. Do this now.
	
	fstat(fd, &fileinfo);

	while (fileinfo.st_size % 512 != 0) {

		write(fd, "\0", 1);
		fileinfo.st_size++;

	}

	close(fd);

	return 0;

}

void convert(int input, char *octal, int size) {

	for (int j = 0; j < size; j++) {

		octal[size - j - 1] = (input % 8) + 48;
		input /= 8;

	}

}
