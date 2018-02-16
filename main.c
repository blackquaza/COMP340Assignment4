#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

// Used for the getopt implementation.
static struct option longopts[] = {
	{"help", no_argument, 0, 'h'},
	{"verbose", no_argument, 0, 'v'},
	{"create", no_argument, 0, 'c'},
	{"compress", no_argument, 0, 'z'},
	{"list", no_argument, 0, 'l'},
	{"extract", no_argument, 0, 'x'},
	{"delete", no_argument, 0, 'd'},
	{"rename", no_argument, 0, 'r'},
	{0, 0, 0, 0}
};

// Contains the help message, which is obtained with the options
// -h or --help. It's long, so I don't want it cluttering up the
// main function.
void helpMsg();

// Defined in 'write.c'
// Writes data to a new or existing archive. Does not handle the
// delete and rename types.
int writeArch(char *archive, int index, char **files, int compress, int verbose);

int main(int argc, const char *argv[]) {

	// Start the process by getting all the options entered. Using
	// getopt_long for this.
	int verbose = 0, compress = 0;
	int index;
	char c = getopt_long(argc, argv, ":hvzclxdr", longopts, &index);
	char command = '\0';
	char *archive = NULL;

	while (c != -1) {

		switch (c) {

			// Display the help message and stop running.
			case 'h':
				helpMsg();
				return 0;
			// Set the verbose flag.
			case 'v':
				verbose = 1;
				break;
			// Set the compress flag.
			case 'z':
				compress = 1;
				break;
			case 'c':
			case 'l':
			case 'x':
			case 'd':
			case 'r':
				// In all of the primary cases, just save the selected
				// action. We'll call the appropriate function later.
				if (command != '\0') break;
				command = c;
				break;
			case '?':
			default:
				// Invalid command.
				printf ("This is not a valid option. Use the --help ");
				printf ("option for usage information.\n");
				break;


		}

		// Get the next option, and loop back.
		c = getopt_long(argc, argv, ":hvzclxdr", longopts, &index);

	}

	// If there was no primary option provided, error out.
	if (command == '\0') {

		printf ("No option was provided. Use the --help ");
		printf ("option for usage information.\n");
		return -1;

	}

	// An option was provided but no argument was provided for the file. Error out.
	if (optind == argc) {

		printf ("No file was specified. Use the --help ");
		printf ("option for usage information.\n");
		return -1;

	} else {

		// The first argument is going to be the archive file.
		// Save the file name for later.
		archive = malloc(sizeof(char) * strlen(argv[optind]));
		strcpy(archive, argv[optind]);
		optind++;

	}

	if (verbose) printf ("./dar running in verbose mode.\n");

	// Set up variables to pass them into other functions.
	index = argc - optind;
	char **files = malloc(sizeof(char *) * index);

	if (verbose) printf ("Command %c with archive file %s.\n", command, archive);
	
	for (int i = 0; i < index; i++) {

		files[i] = argv[i + optind];
		if (verbose) printf("File %i: %s\n", i + 1, files[i]);

	}

	//
	switch (command) {

		case 'c':
			return writeArch(archive, index, files, compress, verbose);
		case 'l':
		case 'x':
		case 'd':
		case 'r':
			break;

	}

}

void helpMsg() {

	printf("\
\n\tDakota's Archiver\n\
\n\
General use\n\
\n\
Create a compressed archive:\n\
\t./dar -cz output_file input_1 input_2 ...\n\
List the contents of an archive:\n\
\t./dar -l archive_file\n\
Extract the contents of an archive:\n\
\t./dar -x archive_file target_directory\n\
\n\
Primary options\n\
\tThe archiver must be used with exacly one of these options. If more than one\n\
\toption is provided, only the first one will be used.\n\
\n\
--create, -c\n\
\tSpecifies the target of an archive file to write to. If it does not exist,\n\
\tthe file is created.\n\
\tUsage: ./dar -c output_file input_1 input_2 ...\n\
--list, -l\n\
\tLists the contents of the specified archive file.\n\
\tUsage: ./dar -l archive_file\n\
--extract, -x\n\
\tExtracts the specified archive file into the specified directory.\n\
\tThe file paths are relative.\n\
\tUsage: ./dar -x archive_file target_directory\n\
--delete, -d\n\
\tSpecifies that certain files or directories are to be deleted\n\
\twhen extracting the archive file. This command does not actually\n\
\tremove data from the archive file.\n\
\tUsage: ./dar -d archive_file to_delete1 to_delete2 ...\n\
--rename, -r\n\
\tSpecifies that certain files or directories are to be renamed\n\
\twhen extracting the archive file. Like --delete, data is not\n\
\tremoved from the archive file.\n\
\n\
Secondary options\n\
\n\
--help, -h\n\
\tShows this help message. All other options and arguments are ignored.\n\
--verbose, -v\n\
\tProvides details on what the archiver is doing. Only usable with one of\n\
\tthe primary options.\n\
--compress, -z\n\
\tCompressed files before adding them to an archive. Only usable when\n\
\talso using the --create option.\n\
");

}
