# Google Drive API Client
Client for making Google DRive API calls from the main FUSE backend. Results of the calls are displayed to STDOUT

## How To Use
You can either compile to an executable via `go build main.go...` or run using `go run main.go...`

#### `-t` : the type of API call we want to perform
- only `list` is suppoerted now and is default
    
#### `-f` : the folder ID where we want to perform the API call
- default is `root`
    
## Examples

`go run main.go -t list -f 1uh_vYAT8SrEC2pdf6ETww70MgvouBdTo`

This lists all files in the folder with id "1uh_vYAT8SrEC2pdf6ETww70MgvouBdTo"

`go run main.go -t ls`

Lists all files in the root directory of the given Google Drive account

## Credentials

`credentials.json` - Contains the OAuth 2.0 client ID and client secret for the "app"

`token.json` - Delete this and it will be generetaed for the account you want to connect to (default contains the token for quinntestvt@gmail.com - likely expired though)

