COMPILER = gcc
FUSE_FILES = ssfs.c JsonTools.c JsonTools.h
GOOGLE_FILES = main.go
NFS_FILES = main.go
RAM_DISK_FILES = main.go 
JANSSON = "./jansson-2.13/"


build: $(FILESYSTEM_FILES)
	cd src/API/google_drive && go build -o google_drive_client $(GOOGLE_FILES)
	cd src/API/NFS && go build -o nfs_api $(NFS_FILES)
	cd src/API/ramdisk && go build -o ramdisk_client $(RAM_DISK_FILES)
	cd FuseImplementation && $(COMPILER) $(FUSE_FILES) -o fuse -D_FILE_OFFSET_BITS=64  `pkg-config fuse --cflags --libs` -ljansson -ggdb3
	echo 'To Mount: ./fuse -f [mount point]'

getFile: getFileFromLocalDirectory.c
	$(COMPILER) -ggdb3 getFileFromLocalDirectory.c -o getFile -ljansson

clean all:
	cd FuseImplementation && rm fuse
	cd FuseImplementation && rm getFile
	cd FuseImplementation && rm errors.txt
	cd src/API/google_drive && rm token.json
	cd src/API/google_drive && rm google_drive_client
	cd src/API/NFS && rm nfs_api
	cd src/API/ramdisk && rm ramdisk_client



cleanJan:
	rm -rf $(JANSSON)
