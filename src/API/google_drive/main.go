package main

import (
	"bufio"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"strings"
	"types"

	"github.com/gabriel-vasile/mimetype"
	"golang.org/x/oauth2"
	"golang.org/x/oauth2/google"
	"google.golang.org/api/drive/v3"
	"google.golang.org/api/option"
)

// Starter OAuth 2.0 code from: https://developers.google.com/drive/api/quickstart/go

// TODO: https://dev.to/douglasmakey/oauth2-example-with-go-3n8a
// https://www.youtube.com/watch?v=OdyXIi6DGYw&ab_channel=AlexPliutau is better

// Retrieve a token, saves the token, then returns the generated client.
func getClient(config *oauth2.Config) *http.Client {
	// The file token.json stores the user's access and refresh tokens, and is
	// created automatically when the authorization flow completes for the first
	// time.
	tokFile := "token.json"
	tok, err := tokenFromFile(tokFile)
	if err != nil {
		tok = getTokenFromWeb(config)
		saveToken(tokFile, tok)
	}
	return config.Client(context.Background(), tok)
}

// Request a token from the web, then returns the retrieved token.
func getTokenFromWeb(config *oauth2.Config) *oauth2.Token {
	authURL := config.AuthCodeURL("state-token", oauth2.AccessTypeOffline)
	fmt.Fprintf(os.Stderr, "Go to the following link in your browser then type the "+
		"authorization code: \n%v\n", authURL)

	var authCode string
	if _, err := fmt.Scan(&authCode); err != nil {
		log.Fatalf("Unable to read authorization code %v", err)
	}

	tok, err := config.Exchange(context.TODO(), authCode)
	if err != nil {
		log.Fatalf("Unable to retrieve token from web %v", err)
	}
	return tok
}

// Retrieves a token from a local file.
func tokenFromFile(file string) (*oauth2.Token, error) {
	f, err := os.Open(file)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	tok := &oauth2.Token{}
	err = json.NewDecoder(f).Decode(tok)
	return tok, err
}

// Saves a token to a file path.
func saveToken(path string, token *oauth2.Token) {
	fmt.Fprintf(os.Stderr, "Saving credential file to: %s\n", path)
	f, err := os.OpenFile(path, os.O_RDWR|os.O_CREATE|os.O_TRUNC, 0600)
	if err != nil {
		log.Fatalf("Unable to cache oauth token: %v", err)
	}
	defer f.Close()
	json.NewEncoder(f).Encode(token)
}

func main() {

	ctx := context.Background()
	b, err := os.ReadFile("credentials.json")
	if err != nil {
		log.Fatalf("Unable to read client secret file: %v", err)
	}

	// If modifying these scopes, delete the previously saved token.json file
	config, err := google.ConfigFromJSON(b, drive.DriveScope, drive.DriveReadonlyScope)
	if err != nil {
		log.Fatalf("Unable to parse client secret file to config: %v", err)
	}
	client := getClient(config)

	srv, err := drive.NewService(ctx, option.WithHTTPClient(client))
	if err != nil {
		log.Fatalf("Unable to retrieve Drive client: %v", err)
	}
	fmt.Fprintf(os.Stderr, "Waiting to read\n")
	// loop until we are told to shutdown
	reader, servicing := bufio.NewReader(os.Stdin), true
	//servicing := true
	for servicing {

		// TODO: make generic API Client type with functions like FindFile, GetFiles, UploadFile, DownloadFile, DeleteFile
		// have functions return ErrorCode's so FUSE can know specific failure
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

// help from: https://developers.google.com/drive/api/v2/reference/files/list
// GetFilesInFolder fetches and displays all files in the given folder
func GetFilesInFolder(drv *drive.Service, path string) ([]types.File, error) {

	// get the folder to list files in
	folder, err := FindFile(drv, path)
	if err != nil {
		return nil, err
	}

	var fs []types.File
	pageToken := ""
	for {
		// prepare a query that only gets file names in the given folder
		q := drv.Files.List().Q(fmt.Sprintf("parents in '%s' and trashed=false", folder.Id)).Fields("files(id, name, size, mimeType)")
		// If we have a pageToken set, apply it to the query
		if pageToken != "" {
			q = q.PageToken(pageToken)
		}

		// run the query
		r, err := q.Do()
		if err != nil {
			fmt.Fprintf(os.Stderr, "An error occurred: %v\n", err)
			return fs, err
		}

		// append the files from the given page that was returned and get the next page token
		for _, f := range r.Files {
			fs = append(fs, types.File{Name: f.Name, IsDir: f.MimeType == "application/vnd.google-apps.folder", Size: f.Size})
		}
		pageToken = r.NextPageToken

		// last page has been processed
		if pageToken == "" {
			break
		}
	}
	return fs, nil
}

// help from: https://gist.github.com/tanaikech/19655a8130bac1ba510b29c9c44bbd97
// UploadFile uploads the given file to the given path in the drive
func UploadFile(srv *drive.Service, file, path string) (*drive.File, error) {
	if file == "" {
		return nil, fmt.Errorf("no file given for an 'upload' call")
	}

	// find where to upload our file
	parent, err := FindFile(srv, path)
	if err != nil {
		return nil, err
	}

	// open and stat our file
	f, err := os.Open(file)
	if err != nil {
		return nil, fmt.Errorf("problem openeing '%s'\n%s", file, err)
	}
	defer f.Close()
	fileInfo, err := f.Stat()
	if err != nil {
		return nil, fmt.Errorf("problem statting '%s'\n%s", file, err)
	}

	// get the MIME type of our file
	mimetype.SetLimit(0)
	mType, err := mimetype.DetectFile(file)
	if err != nil {
		return nil, fmt.Errorf("problem detecting MIME type for '%s'\n%s", file, err)
	}

	// create the drive file to upload/update
	driveFile := &drive.File{Name: fileInfo.Name()}

	// if file already exists, overwrite it
	if res, err := srv.Files.List().Q(fmt.Sprintf("name='%s' and parents in '%s' and trashed=false", f.Name(), parent.Id)).Fields("files(id, name, size, mimeType)").Do(); err == nil && res != nil && res.Files != nil && len(res.Files) > 0 {

		// attempt to update/overwrite the file (might have to add more at)
		return srv.Files.Update(res.Files[0].Id, driveFile).Media(f).ProgressUpdater(func(now, size int64) { fmt.Fprintf(os.Stderr, "%d, %d\r", now, size) }).Do()
	}

	// attempt to upload the file
	driveFile.Parents = []string{parent.Id}
	driveFile.MimeType = mType.String()
	return srv.Files.Create(driveFile).Media(f).ProgressUpdater(func(now, size int64) { fmt.Fprintf(os.Stderr, "%d, %d\r", now, size) }).Do()
}

// DeleteFile deletes the file of folder (and all files in it) at the given path
func DeleteFile(srv *drive.Service, path string) error {

	// find the file we want to delete
	file, err := FindFile(srv, path)
	if err != nil {
		return err
	}

	// we cannot delete the root
	if file.Id == "root" {
		return fmt.Errorf("cannot delete root")
	}

	err = srv.Files.Delete(file.Id).Do()
	if err != nil {
		return err
	}
	return nil
}

// DOES NOT WORK
func DownloadFile(srv *drive.Service, filePath, path string) (*drive.File, error) {
	if filePath == "" {
		return nil, fmt.Errorf("no file given for an 'download' call")
	}

	// find what file to download
	file, err := FindFile(srv, filePath)
	if err != nil {
		return nil, err
	}

	res, err := srv.Files.Get(file.Id).Download()
	//res, err := srv.Files.Export(file.Id, file.MimeType).Download()
	if err != nil {
		return nil, err
	}
	defer res.Body.Close()

	localFile, err := os.Create(path + file.Name)
	if err != nil {
		return nil, err
	}
	defer localFile.Close()

	_, err = io.Copy(localFile, res.Body)
	if err != nil {
		return nil, err
	}

	return file, nil
}

// FindFile find the file/folder given by path
// Path Format: "dir/.../dir/{file,dir}" or just "{file,dir}" if in Google Drive root
func FindFile(srv *drive.Service, path string) (*drive.File, error) {

	// return the root if that is what we are looking for
	if path == "root" || path == "" || path == "/" {
		return &drive.File{Id: "root"}, nil
	}

	// remove leading / if necessary
	if string(path[0]) == "/" {
		path = path[1:]
	}

	// remove trailing / if necessary
	if string(path[len(path)-1]) == "/" {
		path = path[:len(path)-1]
	}

	parents := strings.Split(path, "/")
	target := parents[len(parents)-1]

	// loop through dirs to find where the target resides
	parent := "root"
	for i := 0; i < len(parents)-1; i++ {

		// get the current folder using the previous parent ID
		res, err := srv.Files.List().Q(fmt.Sprintf("name='%s' and parents in '%s' and trashed=false", parents[i], parent)).Fields("files(id)").Do()

		// Exit if error or no results
		if err != nil || (res != nil && res.Files != nil && len(res.Files) == 0) {
			return nil, fmt.Errorf("could not get folder '%s'\n%s", parents[i], err)
		}
		parent = res.Files[0].Id
	}

	// Get target in the current folder
	res, err := srv.Files.List().Q(fmt.Sprintf("name='%s' and parents in '%s' and trashed=false", target, parent)).Fields("files(id, name, size, mimeType)").Do()
	if err != nil || (res != nil && res.Files != nil && len(res.Files) == 0) {
		return nil, fmt.Errorf("could not get folder/file '%s'\n%s", target, err)
	}

	return res.Files[0], nil
}
