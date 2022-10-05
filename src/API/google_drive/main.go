package main

import (
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

type GoogleDriveClient struct {
	types.APIClient
	Srv *drive.Service
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
	httpClient := getClient(config)

	srv, err := drive.NewService(ctx, option.WithHTTPClient(httpClient))
	if err != nil {
		log.Fatalf("Unable to retrieve Drive client: %v", err)
	}

	types.Serve(&GoogleDriveClient{Srv: srv})
}

// help from: https://developers.google.com/drive/api/v2/reference/files/list
// GetFilesInFolder fetches and displays all files in the given folder
func (c *GoogleDriveClient) GetFiles(path string) ([]types.File, error) {

	// get the folder to list files in
	folder, err := c.FindFile(path)
	if err != nil {
		return nil, err
	}

	var fs []types.File
	pageToken := ""
	for {
		// prepare a query that only gets file names in the given folder
		q := c.Srv.Files.List().Q(fmt.Sprintf("parents in '%s' and trashed=false", folder.Id)).Fields("files(id, name, size, mimeType)")
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
func (c *GoogleDriveClient) UploadFile(file, path string) error {
	if file == "" {
		return fmt.Errorf("no file given for an 'upload' call")
	}

	// find where to upload our file
	parent, err := c.FindFile(path)
	if err != nil {
		return err
	}

	// open and stat our file
	f, err := os.Open(file)
	if err != nil {
		return fmt.Errorf("problem openeing '%s'\n%s", file, err)
	}
	defer f.Close()
	fileInfo, err := f.Stat()
	if err != nil {
		return fmt.Errorf("problem statting '%s'\n%s", file, err)
	}

	// get the MIME type of our file
	mimetype.SetLimit(0)
	mType, err := mimetype.DetectFile(file)
	if err != nil {
		return fmt.Errorf("problem detecting MIME type for '%s'\n%s", file, err)
	}

	// create the drive file to upload/update
	driveFile := &drive.File{Name: fileInfo.Name()}

	// if file already exists, overwrite it
	if res, err := c.Srv.Files.List().Q(fmt.Sprintf("name='%s' and parents in '%s' and trashed=false", f.Name(), parent.Id)).Fields("files(id, name, size, mimeType)").Do(); err == nil && res != nil && res.Files != nil && len(res.Files) > 0 {

		// attempt to update/overwrite the file (might have to add more at)
		_, err = c.Srv.Files.Update(res.Files[0].Id, driveFile).Media(f).ProgressUpdater(func(now, size int64) { fmt.Fprintf(os.Stderr, "%d, %d\r", now, size) }).Do()
		return err
	}

	// attempt to upload the file
	driveFile.Parents = []string{parent.Id}
	driveFile.MimeType = mType.String()
	_, err = c.Srv.Files.Create(driveFile).Media(f).ProgressUpdater(func(now, size int64) { fmt.Fprintf(os.Stderr, "%d, %d\r", now, size) }).Do()
	return err
}

// DeleteFile deletes the file of folder (and all files in it) at the given path
func (c *GoogleDriveClient) DeleteFile(path string) error {

	// find the file we want to delete
	file, err := c.FindFile(path)
	if err != nil {
		return err
	}

	// we cannot delete the root
	if file.Id == "root" {
		return fmt.Errorf("cannot delete root")
	}

	err = c.Srv.Files.Delete(file.Id).Do()
	if err != nil {
		return err
	}
	return nil
}

// DOES NOT WORK
func (c *GoogleDriveClient) DownloadFile(filePath, downloadPath string) error {
	if filePath == "" {
		return fmt.Errorf("no file given for an 'download' call")
	}

	// find what file to download
	file, err := c.FindFile(filePath)
	if err != nil {
		return err
	}

	res, err := c.Srv.Files.Get(file.Id).Download()
	//res, err := srv.Files.Export(file.Id, file.MimeType).Download()
	if err != nil {
		return err
	}
	defer res.Body.Close()

	localFile, err := os.Create(downloadPath + file.Name)
	if err != nil {
		return err
	}
	defer localFile.Close()

	_, err = io.Copy(localFile, res.Body)
	if err != nil {
		return err
	}

	return nil
}

// FindFile find the file/folder given by path
// Path Format: "dir/.../dir/{file,dir}" or just "{file,dir}" if in Google Drive root
func (c *GoogleDriveClient) FindFile(path string) (*drive.File, error) {

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
		res, err := c.Srv.Files.List().Q(fmt.Sprintf("name='%s' and parents in '%s' and trashed=false", parents[i], parent)).Fields("files(id)").Do()

		// Exit if error or no results
		if err != nil || (res != nil && res.Files != nil && len(res.Files) == 0) {
			return nil, fmt.Errorf("could not get folder '%s'\n%s", parents[i], err)
		}
		parent = res.Files[0].Id
	}

	// Get target in the current folder
	res, err := c.Srv.Files.List().Q(fmt.Sprintf("name='%s' and parents in '%s' and trashed=false", target, parent)).Fields("files(id, name, size, mimeType)").Do()
	if err != nil || (res != nil && res.Files != nil && len(res.Files) == 0) {
		return nil, fmt.Errorf("could not get folder/file '%s'\n%s", target, err)
	}

	return res.Files[0], nil
}

func (c *GoogleDriveClient) Fprintf(w io.Writer, format string, a ...any) (n int, err error) {
	return fmt.Fprintf(w, fmt.Sprintf("[Google Drive] %s", format), a...)
}
