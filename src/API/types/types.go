package types

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"os"
)

// represnts the input JSON to the API clients
type Command struct {
	Type  string   `json:"command"`
	Path  string   `json:"path"`
	Files []string `json:"files"`
}

// the valid file output for the API clients
type File struct {
	Name  string `json:"Name"`
	IsDir bool   `json:"IsDir"`
	Size  int64  `json:"Size"`
}

// API client error codes
type ErrorCode int

const (
	NO_ERROR ErrorCode = iota
	CLIENT_FAILED
	COMMAND_FAILED
	INVALID_COMMAND
	INVALID_INPUT
	INVALID_FILE
	EOF
)

// Contains a message describing the error and an ErrorCode for the user
type APIError struct {
	Code    ErrorCode
	Message string
}

// API client responses
type Response struct {
	ErrCode ErrorCode // the error code for this response
	Files   []File    // the files to respond with (set to nil to return the ErrorCode)
}

// Returns the string representation of a Response (empty string on marshal error)
func (r *Response) String() string {
	if r.Files == nil {
		return fmt.Sprintf("{%d}", r.ErrCode)
	}
	fileJSON, err := json.Marshal(r.Files)
	if err != nil {
		return ""
	}
	return string(fileJSON)
}

// Functions needed to satisfy API requests and debug
type APIClientInterface interface {
	GetFiles(path string) ([]File, error)
	UploadFile(file, path string) error
	DeleteFile(path string) error
	DownloadFile(filePath, downloadPath string) error
	Fprintf(w io.Writer, format string, a ...any) (n int, err error)
}

type APIClient struct {
	Client APIClientInterface
}

func (api *APIClient) Serve() {

	api.Client.Fprintf(os.Stderr, "Starting to serve...\n")

	reader, servicing := bufio.NewReader(os.Stdin), true
	//servicing := true
	for servicing {

		// TODO: have functions return ErrorCode's so we can output to FUSE
		cmd, response := Command{}, Response{ErrCode: 0, Files: nil}

		// read and check if pipe has been closed
		line, err := reader.ReadString('\n')
		if err != nil {
			api.Client.Fprintf(os.Stderr, "Issue reading:\n%s\n", err)
			cmd.Type = "shutdown"
			response.ErrCode = EOF
		}
		//line := `{"command":"download","path":"","files":["Wage.jpg"]}`

		// parse JSON into struct
		err = json.Unmarshal([]byte(line), &cmd)
		if cmd.Type != "shutdown" && err != nil {
			api.Client.Fprintf(os.Stderr, "Issue unmarshaling:\n%s\n", err)
			response.ErrCode = INVALID_INPUT
			cmd.Type = "err"
		}

		// determine the type of API call we wan to perform
		switch cmd.Type {
		case "list":
			// we want to list files in the given folder
			files, err := api.Client.GetFiles(cmd.Path)
			if err != nil {
				api.Client.Fprintf(os.Stderr, "Error getting files from folder %s:\n%s\n", cmd.Path, err)
				response.ErrCode = COMMAND_FAILED
				break
			}
			api.Client.Fprintf(os.Stderr, "Successfully retrieved %d files from '%s'\n", len(files), cmd.Path)

			// log all file names and form JSON response
			response.Files = files

		case "upload":
			// we want to upload the given files to the given path
			for _, f := range cmd.Files {
				err := api.Client.UploadFile(f, cmd.Path)
				if err != nil {
					api.Client.Fprintf(os.Stderr, "Error uploading '%s':\n%s\n", f, err)
					response.ErrCode = INVALID_FILE
					break
				}
				api.Client.Fprintf(os.Stderr, "Successfully uploaded '%s'\n", f)
			}

		case "download":
			// we want to download the given files to the given path
			for _, f := range cmd.Files {
				err := api.Client.DownloadFile(f, cmd.Path)
				if err != nil {
					api.Client.Fprintf(os.Stderr, "Error downloading '%s':\n%s\n", f, err)
					response.ErrCode = INVALID_FILE
					break
				}
				api.Client.Fprintf(os.Stderr, "Successfully downloaded '%s'\n", f)
			}

		case "delete":
			// we want to delete the given files
			for _, f := range cmd.Files {
				err = api.Client.DeleteFile(f)
				if err != nil {
					api.Client.Fprintf(os.Stderr, "Error deleting '%s':\n%s\n", f, err)
					response.ErrCode = COMMAND_FAILED
					break
				}
				api.Client.Fprintf(os.Stderr, "Successfully deleted '%s'\n", f)
			}

		case "shutdown":
			// stop servicing API calls
			api.Client.Fprintf(os.Stderr, "Shutting down...\n")
			servicing = false

		case "err":
			// do nothing - we want to print the error

		default:
			api.Client.Fprintf(os.Stderr, "Invalid API call type '%s'\n", cmd.Type)
			response.ErrCode = INVALID_COMMAND
		}

		fmt.Println(response.String())
	}
}
