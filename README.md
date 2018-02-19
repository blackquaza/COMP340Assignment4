
	Dakota's Archiver

General use

Create a compressed archive:
	./dar -cz output_file input_1 input_2 ...
List the contents of an archive:
	./dar -l archive_file
Extract the contents of an archive:
	./dar -x archive_file target_directory

Primary options
	The archiver must be used with exacly one of these options. If more than one
	option is provided, only the first one will be used.

--create, -c
	Specifies the target of an archive file to write to. If it does not exist,
	the file is created.
	Usage: ./dar -c output_file input_1 input_2 ...
--list, -l
	Lists the contents of the specified archive file.
	Usage: ./dar -l archive_file
--extract, -x
	Extracts the specified archive file into the specified directory.
	The file paths are relative.
	Usage: ./dar -x archive_file target_directory
--delete, -d
	Specifies that certain files or directories are to be deleted
	when extracting the archive file. This command does not actually
	remove data from the archive file.
	Usage: ./dar -d archive_file to_delete1 to_delete2 ...
--rename, -r
	Specifies that certain files or directories are to be renamed
	when extracting the archive file. Like --delete, data is not
	removed from the archive file.
	Usage: ./dar -r archive_file to_rename1 new_name1 ...

Secondary options

--help, -h
	Shows this help message. All other options and arguments are ignored.
--verbose, -v
	Provides details on what the archiver is doing. Only usable with one of
	the primary options.
