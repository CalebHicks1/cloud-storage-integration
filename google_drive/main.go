package main

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"net/http"
	"os"

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
	dir := flag.String("d", "root", "the directory to perform the API call in")
	filePath := flag.String("f", "", "the file to perform the API call on")
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

	switch *callType {
	// we want to list files in the given folder
	case "list":
		files, err := GetFilesInFolder(srv, *dir)
		if err != nil {
			Exitf("Error getting files from folder %s:\n%s", *dir, err)
		}
		// print all file names to STDOUT
		for _, f := range files {
			fmt.Println(f.Name)
		}
	// we want to uplaod the given file
	// help from: https://gist.github.com/tanaikech/19655a8130bac1ba510b29c9c44bbd97
	case "upload":
		if *filePath == "" {
			Exitf("No file given for an 'upload' call\n")
		}

		// open and stat our file
		file, err := os.Open(*filePath)
		if err != nil {
			Exitf("Problem openeing '%s'\n%s", *filePath, err)
		}
		defer file.Close()
		fileInfo, err := file.Stat()
		if err != nil {
			Exitf("Problem statting '%s'\n%s", *filePath, err)
		}

		// get the MIME type of our file
		mimetype.SetLimit(0)
		mType, err := mimetype.DetectFile(*filePath)
		if err != nil {
			Exitf("Problem detecting MIME type for '%s'\n%s", *filePath, err)
		}

		// attempt to upload the file
		f := &drive.File{Name: fileInfo.Name(), MimeType: mType.String()}
		res, err := srv.Files.Create(f).Media(file).ProgressUpdater(func(now, size int64) { fmt.Printf("%d, %d\r", now, size) }).Do()
		if err != nil {
			Exitf("Problem uploading '%s' to Google Drive\n%s", *filePath, err)
		}
		fmt.Printf("File '%s' uploaded (%s)\n", *filePath, res.Id)
	default:
		Exitf("Invalid API call type '%s'\n", *callType)
	}
}

// https://developers.google.com/drive/api/v2/reference/files/list
// GetFilesInFolder fetches and displays all files in the given folder
func GetFilesInFolder(d *drive.Service, folder string) ([]*drive.File, error) {
	var fs []*drive.File
	pageToken := ""
	for {
		// prepare a query that only gets file names in the given folder
		q := d.Files.List().Q(fmt.Sprintf("parents in '%s'", folder)).Fields("nextPageToken, files(name)")

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

// Exitf performs a printf with given params and then exits
func Exitf(format string, a ...any) {
	fmt.Printf(format, a...)
	os.Exit(1)
}
