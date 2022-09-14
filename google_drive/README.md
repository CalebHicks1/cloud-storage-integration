# Google Drive API Client
Client for making Google DRive API calls from the main FUSE backend. Results of the calls are displayed to STDOUT

## How To Use
You can either compile to an executable via `go build main.go...` or run using `go run main.go...`

#### `-t` : the type of API call we want to perform
- `list` (default) and `upload` are suppoerted now
    
#### `-d` : the directory ID where we want to perform the API call
- default is `root`

#### `-f` : the path to the file we want to upload
- required if using `-t upload`
    
## Examples

`go run main.go -t list -d 1uh_vYAT8SrEC2pdf6ETww70MgvouBdTo`

This lists all files in the folder with id "1uh_vYAT8SrEC2pdf6ETww70MgvouBdTo"

`go run main.go -t list`

Lists all files in the root directory of the given Google Drive account

`go run main.go -t upload -f test_file.txt -d 1uh_vYAT8SrEC2pdf6ETww70MgvouBdTo`

Uploads `test_file.txt` to the folder with id "1uh_vYAT8SrEC2pdf6ETww70MgvouBdTo"

## Credentials

`credentials.json` - Contains the OAuth 2.0 client ID and client secret for the "app"

`token.json` - Delete this and it will be generetaed for the account you want to connect to and let me know what email to add as a test user (default contains the token for quinntestvt@gmail.com - likely expired though); delete if expired as well

