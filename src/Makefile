COMPILER = gcc
FUSE_FILES = ssfs.c JsonTools.h JsonTools.c Drive.c Drive.h logging.c logging.h api.c api.h
GOOGLE_FILES = main.go
FILESYSTEM_FILES = ssfs.c JsonTools.h JsonTools.c Drive.c Drive.h logging.c list/list.c subdirectories/SubDirectory.c subdirectories/subdir_utils.c logging.h api.c api.h MultiList.c cache_utils.c path_utils.c
JANSSON = "./jansson-2.13/"


build:
	cd FuseImplementation && gcc ssfs.c JsonTools.h JsonTools.c Drive.c Drive.h logging.c list/list.c subdirectories/SubDirectory.c subdirectories/subdir_utils.c logging.h api.c api.h MultiList.c cache_utils.c path_utils.c -o fuse -ggdb3 -D_FILE_OFFSET_BITS=64  `pkg-config fuse --cflags --libs` -ljansson
	cd API/google_drive && go build -o google_drive_client $(GOOGLE_FILES)
	cd API/dropbox && go build -o drop_box $(GOOGLE_FILES)
	echo 'To Mount: ./FuseImplementation/fuse -f [mount point]'

authenticate:
	cd API/google_drive && rm -f token.json
	cd API/google_drive  && python3 auth.py
	cd API/google_drive && cp token.json ../../FuseImplementation/token.json
	cd API/dropbox && rm -f token.txt
	cd API/dropbox && python3 auth.py
	cd API/dropbox && cp token.txt ../../FuseImplementation/token.txt



clean all:
	cd FuseImplementation && rm -f fuse errors.txt token.json token.txt
	cd API/google_drive && rm -f google_drive_client token.json
	cd API/dropbox && rm -f dropbox token.txt



cleanJan:
	rm -rf $(JANSSON)
