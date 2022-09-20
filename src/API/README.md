# API Clients
Google Drive client supported.

## How To Use 
The API clients will communicate with your program via its STDOUT and STDIN. First run the API client for your desired cloud storage service as a child process of your program and set up a Unix pipe that allows you to send JSON data to the STDIN of the client as well as recieve JSON output from the client's STDOUT. 

## Valid API Calls
Below are the valid operations that the API clients support along with their valid input and output.

Valid paths:
- root directory: `"/"`, `""`
- other directories: `"/dir1/dir2/"`, `"/dir1/dir2"`, `"dir1/dir2/"`, `"dir1/dir2"`

Valid file inputs:
- provide the absolute path to the file on your system to be uploaded

Valid file outputs:
- `[{"Name":"file.txt", "IsDir":"false", "Size":"1024"}]`
- `[{"Name":"dir1", "IsDir":"true", "Size":"0"}]`
- the size should be in bytes (0 for folders right now)

### List Files - `list`
How to format a call that lists all files in a directory given by the `path` field.

input JSON = `{"command":"list","path":"<valid_path>","file":""}`

response JSON = `{"<file1>", "<file2>", ...}`, `{"<ERROR_CODE>"}` (on error)

### Upload Files - `upload`
How to format a call that uploads a file given by the `file` field to a directory given by the `path` field.

input JSON = `{"command":"upload","path":"<valid_path>","file":"<filename>"}`

response JSON = `{"<ERROR_CODE>"}`

### Delete Files - `delete`
How to format a call that deletes a file/folder (folders recursively delete) given by the `path` field.

input JSON = `{"command":"delete","path":"<valid_path>","file":""}`

response JSON = `{"<ERROR_CODE>"}`

### Shutdown Client - `shutdown`
How to terminate a API client.

input JSON = `{"command":"shutdown","path":"","file":""}`

response JSON = `{"<ERROR_CODE>"}`

## Error Codes

### 0 - No Error
Command succeeded.

### 1 - Client Failed
General error that tells the parent that the client failed.

### 2 - Command Failed
General command related error that tells the parent that the command failed.

### 3 - Invalid Command
Tells the parent that the value of the `command` key is invalid.

### 4 - Invalid Input
Tells the parent that the input passed to STDIN couldn't be parsed.
