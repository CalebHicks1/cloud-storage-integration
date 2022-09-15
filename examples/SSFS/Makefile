COMPILER = gcc
FILESYSTEM_FILES = ssfs.c

build: $(FILESYSTEM_FILES)
	$(COMPILER) $(FILESYSTEM_FILES) -o fuse -D_FILE_OFFSET_BITS=64 -ggdb3 `pkg-config fuse --cflags --libs` -ljansson
	echo 'To Mount: ./fuse -f [mount point]'

	$(COMPILER) getFileFromLocalDirectory.c -o getFile -ljansson


getFile: getFileFromLocalDirectory.c
	$(COMPILER) getFileFromLocalDirectory.c -o getFile -ljansson

clean:
	rm fuse
	rm getFile

