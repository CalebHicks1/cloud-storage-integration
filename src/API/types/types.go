package types

import (
	"encoding/json"
	"fmt"
)

// represnts the input JSON to the API clients
type Command struct {
	Type string `json:"command"`
	Path string `json:"path"`
	File string `json:"file"`
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
)

// API client responses
type Response struct {
	ErrCode ErrorCode // the error code for this response
	Files   []File    // the files to respond with (set to nil to return the ErrorCode)
}

// Returns the string representation of a Response (empty string on marshal error)
func (r *Response) String() string {
	if r.Files == nil {
		return fmt.Sprintf(`{"%d"}`, r.ErrCode)
	}
	fileJSON, err := json.Marshal(r.Files)
	if err != nil {
		return ""
	}
	return string(fileJSON)
}
