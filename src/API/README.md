# API Clients
Google Drive client supported.

## How To Use 
The API clients will communicate with your program via its STDOUT and STDIN. First run the API client for your desired cloud storage service as a child process of your program and set up a Unix pipe that allows you to send JSON data to the STDIN of the client as well as recieve JSON output from the client's STDOUT. 

## Valid API Calls
Below are the valid operations that the API clients support along with their valid input and output.

Valid paths:
- root directory: `"/"`, `""`
- other directories: `"/dir1/dir2"`, `"dir1/dir2"`, `"/dir1/dir2/"`, `"dir1/dir2"`

Valid filenames:
- provide the absolute path to the file on your system to be uploaded

### List Files - `list`
How to format a call that lists all files in a directory given by the `path` field.

input JSON = `{"command":"list","path":"<valid_path>","file":""}`

response JSON = `{"<filename1>", "<filename2>", ...}`, `{"<ERROR_CODE>"}`

### Upload Files - `upload`
How to format a call that uploads a file given by the `file` field to a directory given by the `path` field.

input JSON = `{"command":"upload","path":"<valid_path>","file":"<filename>"}`

response JSON = `{"SUCCESS"}`, `{"<ERROR_CODE>"}`

### Delete Files - `delete`
How to format a call that deletes a file/folder (folders recursively delete) given by the `path` field.

input JSON = `{"command":"delete","path":"<valid_path>","file":""}`

response JSON = `{"SUCCESS"}`, `{"<ERROR_CODE>"}`

## Error Codes

### 1 - Command Failed
General error that tells the filesystem that the command failed.
