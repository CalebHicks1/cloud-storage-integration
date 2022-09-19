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
echo '{"command":"list", "path":"/", "file":"example"}' | sudo ./nfs_api
```
Or execute `nfs_api` then write the json arg:
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

# Commands
## List
Input:
```
{"command":"list","path":"/","file":""}
```
Output
```
[{"Name":"dir1","IsDir":true},{"Name":"file3","IsDir":false},{"Name":"hello.txt","IsDir":false}]
```
Input:
```
{"command":"list","path":"dir1","file":""}
```
Output:
```
[{"Name":"file1","IsDir":false},{"Name":"file2","IsDir":false}]
```