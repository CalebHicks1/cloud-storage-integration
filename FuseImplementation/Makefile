COMPILER = gcc
FILESYSTEM_FILES = ssfs.c JsonTools.h JsonTools.c Drive.c Drive.h logging.c list/list.c subdirectories/SubDirectory.c subdirectories/subdir_utils.c logging.h api.c api.h MultiList.c
JANSSON = "./jansson-2.13/"


build: $(FILESYSTEM_FILES)
	$(COMPILER) $(FILESYSTEM_FILES) -o fuse -ggdb3 -D_FILE_OFFSET_BITS=64  `pkg-config fuse --cflags --libs` -ljansson
	echo 'To Mount: sudo ./fuse -f [mount point]'

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

