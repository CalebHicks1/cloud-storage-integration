# Google Drive API Client
Client for making Google DRive API calls from the main FUSE backend. Requests for API calls are to be sent to STDIN and results of the calls are printed to STDOUT.

## How To Use

See the `README.md` in the `API` folder to see how to send requests and recieve responses to and from the API client.
    
## Examples

`go run main.go`

This starts up the API client. You can also compile with the `Makefile` by running `make` and run the following:

`./google_drive_client`

## Credentials

`credentials.json` - Contains the OAuth 2.0 client ID and client secret for the "app"

`token.json` - Delete this and it will be generetaed for the account you want to connect to and let me know what email to add as a test user (default contains the token for quinntestvt@gmail.com - likely expired though); delete if expired as well

