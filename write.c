#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define _POSIX_C_SOURCE

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

int writeArch (char *archive, int index, char *files[], int compress, int verbose) {

	printf ("%s\n", archive);
	// Open or create the archive. Quit if there's an error.
	int fd = open(archive, O_RDWR, O_CREAT);
	free(archive);

	printf ("%i\n", fd);

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

	printf ("lseek %i\n", fileinfo.st_size - TR_SIZE);

	if (verbose) printf ("Archive file opened.\n");

	for (int i = 0; i < index; i++) {

		fstatat(AT_FDCWD, files[i], &fileinfo, 0);

		if (verbose) printf ("Writing: %s", files[i]);

		// Creating a string to hold the concatinated header.
		char *header = malloc((76 + strlen(files[i])) * sizeof(char));
		char conv[11];
		strcpy(header, "070707");

		sprintf(conv, "%06i", fileinfo.st_dev);
		strcat(header, conv);
		sprintf(conv, "%06i", fileinfo.st_ino);
		strcat(header, conv);

		// st_mode provides a decimal version, but odc wants the octal
		// version. Convert the decimal to octal before adding it.
		//
		// Refered to the following webpage for conversion information:
		// http://man7.org/linux/man-pages/man7/inode.7.html
		// 
		// Output: First three numbers are the file type. 100 for normal and
		// 040 for directories. Last three numbers are permissions.
		int temp = fileinfo.st_mode;
		char octal[6];

		for (int j = 0; j < 6; j++) {

			octal[5-j] = (temp % 8) + 48;
			temp /= 8;

		}

		//sprintf(conv, "%06i", fileinfo.st_mode);
		strcat(header, octal);
		printf ("raw %i conv %s\n", fileinfo.st_mode, octal);
		sprintf(conv, "%06i", fileinfo.st_uid);
		strcat(header, conv);
		sprintf(conv, "%06i", fileinfo.st_gid);
		strcat(header, conv);
		sprintf(conv, "%06i", fileinfo.st_nlink);
		strcat(header, conv);
		sprintf(conv, "%06i", fileinfo.st_rdev);
		strcat(header, conv);
		sprintf(conv, "%011i", fileinfo.st_mtime);
		strcat(header, conv);
		sprintf(conv, "%06i", strlen(files[i]) + 1);
		strcat(header, conv);
		sprintf(conv, "%011i", fileinfo.st_size + strlen(files[i]) + 3);
		strcat(header, conv);
		strcat(header, files[i]);
		strcat(header, "\0");

		// Write the header to the file
		write(fd, header, 76 + strlen(files[i]) + 1);

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
