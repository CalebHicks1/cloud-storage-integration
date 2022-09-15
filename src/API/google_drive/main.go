package main

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"net/http"
	"os"
	"strings"

	"github.com/gabriel-vasile/mimetype"
	"golang.org/x/oauth2"
	"golang.org/x/oauth2/google"
	"google.golang.org/api/drive/v3"
	"google.golang.org/api/option"
)

// Starter code from: https://developers.google.com/drive/api/quickstart/go

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
	fmt.Printf("Go to the following link in your browser then type the "+
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
	fmt.Printf("Saving credential file to: %s\n", path)
	f, err := os.OpenFile(path, os.O_RDWR|os.O_CREATE|os.O_TRUNC, 0600)
	if err != nil {
		log.Fatalf("Unable to cache oauth token: %v", err)
	}
	defer f.Close()
	json.NewEncoder(f).Encode(token)
}

func main() {

	// parse command line args
	callType := flag.String("t", "list", "the type of API call we want to perform")
	path := flag.String("p", "root", "the path to perform the API call in/on")
	file := flag.String("f", "", "the file to perform the API call on")
	flag.Parse()

	ctx := context.Background()
	b, err := os.ReadFile("credentials.json")
	if err != nil {
		log.Fatalf("Unable to read client secret file: %v", err)
	}

	// If modifying these scopes, delete the previously saved token.json file
	config, err := google.ConfigFromJSON(b, drive.DriveScope)
	if err != nil {
		log.Fatalf("Unable to parse client secret file to config: %v", err)
	}
	client := getClient(config)

	srv, err := drive.NewService(ctx, option.WithHTTPClient(client))
	if err != nil {
		log.Fatalf("Unable to retrieve Drive client: %v", err)
	}

	// determine the type of API call we wan to perform
	switch *callType {
	case "list":
		// we want to list files in the given folder
		files, err := GetFilesInFolder(srv, *path)
		if err != nil {
			Exitf("Error getting files from folder %s:\n%s", *path, err)
		}
		// print all file names to STDOUT
		for _, f := range files {
			fmt.Println(f.Name)
		}
	case "upload":
		// we want to uplaod the given file
		res, err := UploadFile(srv, *file, *path)
		if err != nil {
			Exitf("Problem uploading '%s' to Google Drive\n%s", *file, err)
		}
		fmt.Printf("File '%s' uploaded (%s)\n", *file, res.Id)
	case "delete":
		err = DeleteFile(srv, *path)
		if err != nil {
			Exitf("Error deleting '%s'\n%s", *path, err)
		}
		fmt.Printf("Successfully deleted '%s'\n", *path)
	default:
		Exitf("Invalid API call type '%s'\n", *callType)
	}
}

// https://developers.google.com/drive/api/v2/reference/files/list
// GetFilesInFolder fetches and displays all files in the given folder
func GetFilesInFolder(drv *drive.Service, path string) ([]*drive.File, error) {

	// get the folder to list files in
	folder, err := FindFile(drv, path)
	if err != nil {
		return nil, err
	}

	var fs []*drive.File
	pageToken := ""
	for {
		// prepare a query that only gets file names in the given folder
		q := drv.Files.List().Q(fmt.Sprintf("parents in '%s' and trashed=false", folder.Id))
		// If we have a pageToken set, apply it to the query
		if pageToken != "" {
			q = q.PageToken(pageToken)
		}

		// run the query
		r, err := q.Do()
		if err != nil {
			fmt.Printf("An error occurred: %v\n", err)
			return fs, err
		}

		// append the files from the given page that was returned and get the next page token
		fs = append(fs, r.Files...)
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

	// attempt to upload the file
	driveFile := &drive.File{Name: fileInfo.Name(), MimeType: mType.String(), Parents: []string{parent.Id}}
	return srv.Files.Create(driveFile).Media(f).ProgressUpdater(func(now, size int64) { fmt.Printf("%d, %d\r", now, size) }).Do()
}

// DeleteFile deletes the file of folder (and all files in it) at the given path
func DeleteFile(srv *drive.Service, path string) error {

	// we cannot delete the root
	if path == "root" {
		return fmt.Errorf("cannot delete root")
	}

	// find the file we want to delete
	file, err := FindFile(srv, path)
	if err != nil {
		return err
	}

	err = srv.Files.Delete(file.Id).Do()
	if err != nil {
		return err
	}

	// attempt to upload the file
	return nil
}

// FindFile find the file/folder given by path
// Path Format: "dir/.../dir/{file,dir}" or just "{file,dir}" if in Google Drive root
func FindFile(srv *drive.Service, path string) (*drive.File, error) {
	if path == "" {
		return nil, fmt.Errorf("empty path")
	}

	// return the root if that is what we are looking for
	if path == "root" {
		return &drive.File{Id: "root"}, nil
	}

	// remove leading / if necessary
	if string(path[0]) == "/" {
		path = path[1:]
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
	res, err := srv.Files.List().Q(fmt.Sprintf("name='%s' and parents in '%s' and trashed=false", target, parent)).Do()
	if err != nil || (res != nil && res.Files != nil && len(res.Files) == 0) {
		return nil, fmt.Errorf("could not get folder/file '%s'\n%s", target, err)
	}

	return res.Files[0], nil
}

// Exitf performs a printf with given params and then exits (like log.Fatalf)
func Exitf(format string, a ...any) {
	fmt.Printf(format, a...)
	os.Exit(1)
}
