package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"os"
	"types"

	"github.com/dropbox/dropbox-sdk-go-unofficial/v6/dropbox"
	"github.com/dropbox/dropbox-sdk-go-unofficial/v6/dropbox/users"
)

const TOKEN = "sl.BQiZltjI6NylGdMqa430mQZqyMV-dXhrLXMsVnoOox-Oa8wpMWhbeSfsJkohfwXcFKK7JGqpBzFP6ki_aVJC9K3Ni9Nt46mvDz-P901yGJ7JytPlmSey1nW8ngIsSfUYCw3e8OE"

func main() {
	config := dropbox.Config{
		Token:    TOKEN,
		LogLevel: dropbox.LogInfo, // if needed, set the desired logging level. Default is off
	}
	dbx := users.New(config)

	reader, servicing := bufio.NewReader(os.Stdin), true
	//servicing := true
	for servicing {

		cmd, response := types.Command{}, types.Response{ErrCode: 0, Files: nil}

		// read and check if pipe has been closed
		line, err := reader.ReadString('\n')
		if err != nil {
			fmt.Fprintf(os.Stderr, "Issue reading:\n%s\n", err)
			cmd.Type = "shutdown"
			response.ErrCode = types.EOF
		}
		//line := `{"command":"download","path":"","files":["Wage.jpg"]}`

		// parse JSON into struct
		err = json.Unmarshal([]byte(line), &cmd)
		if cmd.Type != "shutdown" && err != nil {
			fmt.Fprintf(os.Stderr, "Issue unmarshaling:\n%s\n", err)
			response.ErrCode = types.INVALID_INPUT
			cmd.Type = "err"
		}

		// determine the type of API call we wan to perform
		switch cmd.Type {
		case "list":
			// we want to list files in the given folder
			files, err := GetFilesInFolder(srv, cmd.Path)
			if err != nil {
				fmt.Fprintf(os.Stderr, "Error getting files from folder %s:\n%s", cmd.Path, err)
				response.ErrCode = types.COMMAND_FAILED
				break
			}

			// log all file names and form JSON response
			response.Files = files

		case "upload":
			// we want to upload the given files to the given path
			for _, f := range cmd.Files {
				res, err := UploadFile(srv, f, cmd.Path)
				if err != nil {
					fmt.Fprintf(os.Stderr, "Problem uploading '%s' to Google Drive\n%s", f, err)
					response.ErrCode = types.INVALID_FILE
					break
				}
				fmt.Fprintf(os.Stderr, "File '%s' uploaded (%s)\n", f, res.Id)
			}

		case "download":
			// we want to download the given files to the given path
			for _, f := range cmd.Files {
				res, err := DownloadFile(srv, f, cmd.Path)
				if err != nil {
					fmt.Fprintf(os.Stderr, "Problem downloading '%s' from Google Drive\n%s", f, err)
					response.ErrCode = types.INVALID_FILE
					break
				}
				fmt.Fprintf(os.Stderr, "File '%s' downloaded (%s)\n", f, res.Id)
			}

		case "delete":
			// we want to delete the given files
			for _, f := range cmd.Files {
				err = DeleteFile(srv, f)
				if err != nil {
					fmt.Fprintf(os.Stderr, "Error deleting '%s'\n%s", f, err)
					response.ErrCode = types.COMMAND_FAILED
					break
				}
				fmt.Fprintf(os.Stderr, "Successfully deleted '%s'\n", f)
			}

		case "shutdown":
			// stop servicing API calls
			fmt.Fprintln(os.Stderr, "Shutting down...")
			servicing = false

		case "err":
			// do nothing - we want to print the error

		default:
			fmt.Fprintf(os.Stderr, "Invalid API call type '%s'\n", cmd.Type)
			response.ErrCode = types.INVALID_COMMAND
		}

		fmt.Println(response.String())
	}
}
