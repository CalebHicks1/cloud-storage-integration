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
	Name     string `json:"Name"`
	IsDir    bool   `json:"IsDir"`
	Size     uint64 `json:"Size"`
	Modified int64  `json:"Modified"`
}

// API client error codes
type ErrorCode int

const (
	NO_ERROR ErrorCode = iota
	CLIENT_FAILED
	COMMAND_FAILED
	INVALID_COMMAND
	INVALID_INPUT
	FILE_NOT_FOUND
	EOF
)

// Contains a message describing the error and an ErrorCode for the user
type APIError struct {
	Code    ErrorCode `json:"code"`
	Message string    `json:"message"`
}

// Updates the fields of an APIError
func (e *APIError) Update(code ErrorCode, format string, a ...any) {
	e.Code = code
	e.Message = fmt.Sprintf(format, a...)
}

// API client responses
type Response struct {
	Err   *APIError // the error code for this response
	Files []File    // the files to respond with (set to nil to return the ErrorCode)
}

// Returns the string representation of a Response (empty string on marshal error)
func (r *Response) String() string {

	if r.Files == nil {
		errJSON, err := json.Marshal(r.Err)
		if err != nil {
			return ""
		}
		return string(errJSON)
	}
	fileJSON, err := json.Marshal(r.Files)
	if err != nil {
		return ""
	}
	return string(fileJSON)
}

// Functions needed to satisfy API requests and debug
type APIClientInterface interface {
	fmt.Stringer
	GetFiles(path string) ([]File, *APIError)
	UploadFile(file, path string) *APIError
	DeleteFile(path string) *APIError
	DownloadFile(filePath, downloadPath string) *APIError
}

type APIClient struct {
	Client APIClientInterface
}

// when debug=true, additional info is printed to STDERR
func (api *APIClient) Serve(debug bool) {

	if debug {
		api.Fprintf(os.Stderr, "Starting to serve...\n")
	}

	reader, servicing := bufio.NewReader(os.Stdin), true
	//servicing := true
	for servicing {

		// TODO: have functions return ErrorCode's so we can output to FUSE
		cmd, response := Command{}, Response{Err: &APIError{Code: NO_ERROR, Message: "No Error"}, Files: nil}

		// read and check if pipe has been closed
		line, err := reader.ReadString('\n')
		if err != nil {
			cmd.Type = "shutdown"
			response.Err.Update(EOF, "Issue reading:\n%s\n", err)
		}
		//line := `{"command":"download","path":"","files":["Wage.jpg"]}`

		// parse JSON into struct
		err = json.Unmarshal([]byte(line), &cmd)
		if cmd.Type != "shutdown" && err != nil {
			response.Err.Update(INVALID_INPUT, "Issue unmarshaling:\n%s\n", err)
			cmd.Type = "err"
		}

		// determine the type of API call we wan to perform
		switch cmd.Type {
		case "list":
			// we want to list files in the given folder
			files, err := api.Client.GetFiles(cmd.Path)
			if err != nil {
				response.Err.Update(err.Code, "Error getting files from '%s':\n%s\n", cmd.Path, err.Message)
				break
			}
			if debug {
				api.Fprintf(os.Stderr, "Successfully retrieved %d files from '%s'\n", len(files), cmd.Path)
			}

			// log all file names and form JSON response
			response.Files = files

		case "upload":
			// we want to upload the given files to the given path
			for _, f := range cmd.Files {

				// cannot upload nothing
				if f == "" {
					response.Err.Update(INVALID_INPUT, "no file given for an 'upload' call")
					break
				}

				// cannot upload root
				if f == "/" {
					response.Err.Update(INVALID_INPUT, `cannot upload root "/"`)
					break
				}

				err := api.Client.UploadFile(f, cmd.Path)
				if err != nil {
					response.Err.Update(err.Code, "Error uploading '%s':\n%s\n", f, err.Message)
					break
				}
				if debug {
					api.Fprintf(os.Stderr, "Successfully uploaded '%s'\n", f)
				}
			}

		case "download":
			// we want to download the given files to the given path
			for _, f := range cmd.Files {

				// cannot download nothing
				if f == "" {
					response.Err.Update(INVALID_INPUT, "no file given for an 'download' call")
					break
				}

				// cannot download root
				if f == "/" {
					response.Err.Update(INVALID_INPUT, `cannot download root "/"`)
					break
				}

				err := api.Client.DownloadFile(f, cmd.Path)
				if err != nil {
					response.Err.Update(err.Code, "Error downloading '%s':\n%s\n", f, err.Message)
					break
				}
				if debug {
					api.Fprintf(os.Stderr, "Successfully downloaded '%s'\n", f)
				}
			}

		case "delete":
			// we want to delete the given files
			for _, f := range cmd.Files {

				// cannot delete nothing
				if f == "" {
					response.Err.Update(INVALID_INPUT, "no file given for an 'delete' call")
					break
				}

				// cannot delete root
				if f == "/" {
					response.Err.Update(INVALID_INPUT, `cannot delte root "/"`)
					break
				}

				err := api.Client.DeleteFile(f)
				if err != nil {
					response.Err.Update(err.Code, "Error deleting '%s':\n%s\n", f, err.Message)
					break
				}
				if debug {
					api.Fprintf(os.Stderr, "Successfully deleted '%s'\n", f)
				}
			}

		case "shutdown":
			// stop servicing API calls
			if debug {
				api.Fprintf(os.Stderr, "Shutting down...\n")
			}
			servicing = false

		case "err":
			// do nothing - we want to print the error

		default:
			response.Err.Update(INVALID_COMMAND, "Invalid API call type '%s'\n", cmd.Type)
		}

		fmt.Println(response.String())
	}
}

func (api *APIClient) Fprintf(w io.Writer, format string, a ...any) (n int, err error) {
	return fmt.Fprintf(w, fmt.Sprintf("[%s] %s", api.Client.String(), format), a...)
}
