COMPILER = gcc
FILESYSTEM_FILES = ssfs.c JsonTools.c JsonTools.h Drive.c logging.c api.c
JANSSON = "./jansson-2.13/"


build: $(FILESYSTEM_FILES)
	$(COMPILER) $(FILESYSTEM_FILES) -o fuse -ggdb3 -D_FILE_OFFSET_BITS=64  `pkg-config fuse --cflags --libs` -ljansson
	echo 'To Mount: ./fuse -f [mount point]'

	$(COMPILER) getFileFromLocalDirectory.c -o getFile -ljansson -ggdb3


getFile: getFileFromLocalDirectory.c
	$(COMPILER) -ggdb3 getFileFromLocalDirectory.c -o getFile -ljansson

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

