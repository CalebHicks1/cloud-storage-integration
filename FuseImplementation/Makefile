COMPILER = gcc
FILESYSTEM_FILES = ssfs.c
JANSSON = "./jansson-2.13/"

build: $(FILESYSTEM_FILES)
	$(COMPILER) $(FILESYSTEM_FILES) -o fuse -D_FILE_OFFSET_BITS=64 -ggdb3 `pkg-config fuse --cflags --libs` -ljansson
	echo 'To Mount: ./fuse -f [mount point]'

	$(COMPILER) getFileFromLocalDirectory.c -o getFile -ljansson


getFile: getFileFromLocalDirectory.c
	$(COMPILER) getFileFromLocalDirectory.c -o getFile -ljansson

clean:
	rm fuse
	rm getFile

jansson:
	gunzip -c jansson-2.13.tar.gz | tar xf -
	cd $(JANSSON) && ./configure
	cd $(JANSSON) && make 
	cd $(JANSSON) && make check
	cd $(JANSSON) && make install

cleanJan:
	rm -rf $(JANSSON)

