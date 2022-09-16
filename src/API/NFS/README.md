# About
Currently, this program mounts the NFS server at `cap.calebhicks.net`, reads one line from `stdin`, prints the parsed json argument, unmounts the NFS drive, then exits. 

# Build
Compile program:

```
cd NFS
go build .
```

# Run
Example command:
```
echo '{"command":"ls", "path":"/", "file":"example"}' | sudo ./nfs_api
```
Rr execute `nfs_api` then write the json arg:
```
sudo ./nfs_api
{"command":"ls", "path":"/", "file":"example"}
```
# Notes
- The `nfs_api` file must be run as root in order to mount the nfs drive.

# TODO
- [] Read server, mount point configuration from command line(?) on startup.
- [] Read from NFS mount based on json command.
- [] Output valid json response 