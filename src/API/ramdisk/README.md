# Google Drive API Client
Client for initializing and copying to/from a ramdrive. Requests for API calls are to be sent to STDIN and results of the calls are printed to STDOUT.

## How To Use

See the `README.md` in the `API` folder to see how to send requests and recieve responses to and from the API client.
    
## Examples

`go run main.go`

This starts up the API client. You can also compile with the `Makefile` by running `make` and run the following:

`./ramdisk_client`

## Configuration

`config.json` - Configures the ramdisk's size, name, mount path, etc.

