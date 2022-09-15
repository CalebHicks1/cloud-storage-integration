# Google Drive API Client
Client for making Google DRive API calls from the main FUSE backend. Results of the calls are displayed to STDOUT

## How To Use
You can either compile to an executable via `go build main.go...` or run using `go run main.go...`

#### `-t` : the type of API call we want to perform
- `list` is default
- `list`, `upload`, and `delete` are suppoerted now
    
#### `-p` : the path where we want to perform the API call
- default is `root`
- format is "dir/.../dir/{file,dir}" or just "{file,dir}" if in Google Drive root

#### `-f` : the path to the file we want to upload
- required only if using `-t upload`
    
## Examples


`go run main.go -t list`

Lists all files in the root directory of the given Google Drive account (same as running `go run main.go`)

`go run main.go -t list -p Main`

This lists all files in the folder named "Main" in the root Google Drive directory

`go run main.go -t upload -f test_file.txt -p Main/Test`

Uploads `test_file.txt` to the folder "Main/Test" in the root Google Drive directory

`go run main.go -t delete -p Main/Test`

Recursively deletes the folder "Main/Test" in the root Google Drive directory

## Credentials

`credentials.json` - Contains the OAuth 2.0 client ID and client secret for the "app"

`token.json` - Delete this and it will be generetaed for the account you want to connect to and let me know what email to add as a test user (default contains the token for quinntestvt@gmail.com - likely expired though); delete if expired as well

