# cloud-storage-integration
CS 4284 Cloud Storage Filesystem Integration Capstone Project

## Authors and acknowledgment
Alyssa Lowe <alowe15@vt.edu>

Brad Grohs <bgrohs@vt.edu>

Caleb Hicks <hcaleb@vt.edu>

Carson Botelho <carsonb19@vt.edu>

James Gillespie <jamesgillespie@vt.edu>

Quinn Nolan <qjnolan@vt.edu>

## About
cloud-storage-integration is an application to facilitate syncing between cloud storage services and your local machine. Fuse is used to present files stored in the cloud to the user at a specified mount point. Files are stored locally on an as-needed basis, and synced between drives. There are three main components: the frontend, which encapsulates presenting files to the user and organizing them; the APIs, which interact with the APIs of the cloud storage services, and the daemon, which periodically updates the cloud devices with file changes. 

## Requirements
libfuse: We recommend cloning the libfuse respository and installing from there: <https://github.com/libfuse/libfuse.git>. Follow the instructions provided in their README. 

libjannson: The necessary files are provided in in a tar.gz file in src/FuseImplementation:

    $ tar -xvf jansson-2.13.tar.gz
    $ cd jansson-2.13
    $ ./configure
    $ make
    $ make install

## Setup

## Configuration

There is currently no configuration file. In order to select which APIs to use, change the definition of the Drive_Object in ssfs.c: find the line commented "execs", and add / remove paths to the API executables. The paths to these executables are currently:
    
    $ ../API/google_drive/google_drive_client (Google Drive)
    $ ../API/dropbox/drop_box (Dropbox)
    $ ../API/one_drive/bin/Debug/net6.0/publish/OneDrive (OneDrive)
    
Listing an API executable here means you will need to authenticate an account for them in the following step. 

## Building 

Navigate to the source directory. Run make as usual

    $ make

Then run again to generate access tokens:
    $ make authenticate

This will provide a series of urls to navgiate to. On each page, you will be asked to login to the corresponding account and give the application permissions to view and modify your files. 

## Running 

Navigate into src/FuseImplementation, and run

    $ ./fuse -f <your mount point> 

## Disclaimer

This is a project made by students in a short time span as part of our coursework. This is NOT a fully-functional application, nor should it be used as such. 

## License
For open source projects, say how it is licensed.
