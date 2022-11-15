package main

import (
	"context"
	"encoding/base64"
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"net/http"
	"os"
	"time"

	"github.com/goh-chunlin/go-onedrive/onedrive"
	"golang.org/x/oauth2"
	microsoft "golang.org/x/oauth2/microsoft"
)

// type GoogleDriveClient struct {
// 	types.APIClient
// 	Srv  *drive.Service
// 	Name string
// }

var (
	config = &oauth2.Config{
		RedirectURL:  "http://localhost/",
		ClientID:     "47eb8dd1-1d88-4906-bd34-5dc476ec70d4",
		ClientSecret: "Ghu8Q~kvsjFJggsXeOijBA7I9DISA3yGbOSNjc_U",
		Scopes:       []string{"User.Read", "Files.ReadWrite.All"},
		Endpoint:     microsoft.AzureADEndpoint("consumers"),
	}
	state = base64.StdEncoding.EncodeToString([]byte(time.Now().String()))
)

func main() {
	//debug := flag.Bool("debug", false, "prints successs statments for debugging")
	tok_file := flag.String("token_file", "token.json", "file where google token is stored")
	flag.Parse()

	httpClient := getClient(config, *tok_file)

	// fmt.Scanln()
	ctx := context.Background()
	// ts := oauth2.StaticTokenSource(
	// 	&oauth2.Token{AccessToken: "..."},
	// )
	// tc := oauth2.NewClient(ctx, ts)

	client := onedrive.NewClient(httpClient)

	// list all OneDrive drives for the current logged in user
	drives, err := client.Drives.List(ctx)

	if err != nil {
		log.Fatal(err)
	}

	fmt.Printf("Hey here's the drives: %#v\n", drives)

	// // If modifying these scopes, delete the previously saved token.json file
	// config, err := google.ConfigFromJSON(b, drive.DriveScope, drive.DriveReadonlyScope)
	// if err != nil {
	// 	log.Fatalf("Unable to parse client secret file to config: %v", err)
	// }
	// httpClient := getClient(config, *tok_file)

	// srv, err := drive.NewService(ctx, option.WithHTTPClient(httpClient))
	// if err != nil {
	// 	log.Fatalf("Unable to retrieve Drive client: %v", err)
	// }

	// api := types.APIClient{Client: &GoogleDriveClient{Srv: srv, Name: "Google Drive"}}
	// api.Serve(*debug)
}

// Retrieves a token from a local file.
func tokenFromFile(filename string) (*oauth2.Token, error) {
	f, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	tok := &oauth2.Token{}
	err = json.NewDecoder(f).Decode(tok)
	return tok, err
}

// Starter OAuth 2.0 code from: https://developers.google.com/drive/api/quickstart/go

// TODO: https://dev.to/douglasmakey/oauth2-example-with-go-3n8a
// https://www.youtube.com/watch?v=OdyXIi6DGYw&ab_channel=AlexPliutau is better

// Retrieve a token, saves the token, then returns the generated client.
func getClient(config *oauth2.Config, token_file_name string) *http.Client {
	// The file token.json stores the user's access and refresh tokens, and is
	// created automatically when the authorization flow completes for the first
	// time.
	tokFile := token_file_name
	tok, err := tokenFromFile(tokFile)
	if err != nil {
		tok = getTokenFromWeb(config)
		saveToken(tokFile, tok)
	}
	return config.Client(context.Background(), tok)
}

// Request a token from the web, then returns the retrieved token.
func getTokenFromWeb(config *oauth2.Config) *oauth2.Token {
	authURL := config.AuthCodeURL(state, oauth2.AccessTypeOffline)
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

// // help from: https://developers.google.com/drive/api/v2/reference/files/list
// // GetFilesInFolder fetches and displays all files in the given folder
// func (c *GoogleDriveClient) GetFiles(path string) ([]types.File, *types.APIError) {

// 	// get the folder to list files in
// 	folder, err := c.FindFile(path)
// 	if err != nil {
// 		return nil, &types.APIError{Code: types.FILE_NOT_FOUND, Message: fmt.Sprintf("%s\n", err)}
// 	}

// 	var fs []types.File
// 	pageToken := ""
// 	for {
// 		// prepare a query that only gets file names in the given folder
// 		q := c.Srv.Files.List().Q(fmt.Sprintf("parents in '%s' and trashed=false", folder.Id)).Fields("files(id, name, size, mimeType, modifiedTime)")
// 		// If we have a pageToken set, apply it to the query
// 		if pageToken != "" {
// 			q = q.PageToken(pageToken)
// 		}

// 		// run the query
// 		r, err := q.Do()
// 		if err != nil {
// 			return nil, &types.APIError{Code: types.CLIENT_FAILED, Message: fmt.Sprintf("%s\n", err)}
// 		}

// 		// append the files from the given page that was returned and get the next page token
// 		for _, f := range r.Files {

// 			t, err := time.Parse(time.RFC3339, f.ModifiedTime)
// 			if err != nil {
// 				return nil, &types.APIError{Code: types.CLIENT_FAILED, Message: fmt.Sprintf("%s\n", err)}
// 			}

// 			fs = append(fs, types.File{Name: f.Name, IsDir: f.MimeType == "application/vnd.google-apps.folder", Size: uint64(f.Size), Modified: t.Unix()})
// 		}
// 		pageToken = r.NextPageToken

// 		// last page has been processed
// 		if pageToken == "" {
// 			break
// 		}
// 	}
// 	return fs, nil
// }

// // help from: https://gist.github.com/tanaikech/19655a8130bac1ba510b29c9c44bbd97
// // UploadFile uploads the given file to the given path in the drive
// func (c *GoogleDriveClient) UploadFile(file, path string) *types.APIError {

// 	// find where to upload our file
// 	parent, err := c.FindFile(path)
// 	if err != nil {
// 		return &types.APIError{Code: types.FILE_NOT_FOUND, Message: fmt.Sprintf("%s\n", err)}
// 	}

// 	// open and stat our file
// 	f, err := os.Open(file)
// 	if err != nil {
// 		return &types.APIError{Code: types.COMMAND_FAILED, Message: fmt.Sprintf("problem openeing\n%s\n", err)}
// 	}
// 	defer f.Close()
// 	fileInfo, err := f.Stat()
// 	if err != nil {
// 		return &types.APIError{Code: types.COMMAND_FAILED, Message: fmt.Sprintf("problem statting\n%s\n", err)}
// 	}

// 	// create the drive file to upload/update
// 	driveFile := &drive.File{Name: fileInfo.Name()}

// 	// if file already exists, overwrite it
// 	res, err := c.Srv.Files.List().Q(fmt.Sprintf("name='%s' and parents in '%s' and trashed=false", fileInfo.Name(), parent.Id)).Fields("files(id, name, size, mimeType)").Do()
// 	if err == nil && res != nil && res.Files != nil && len(res.Files) > 0 {
// 		// attempt to update/overwrite the file (might have to add more at)
// 		_, err = c.Srv.Files.Update(res.Files[0].Id, driveFile).Media(f).Do()
// 	} else if err == nil {
// 		// attempt to upload the file
// 		driveFile.Parents = []string{parent.Id}
// 		driveFile.MimeType = "application/vnd.google-apps.folder"

// 		if !fileInfo.IsDir() {
// 			// get the MIME type of our file
// 			mimetype.SetLimit(0)
// 			mType, err := mimetype.DetectFile(file)
// 			if err != nil {
// 				return &types.APIError{Code: types.COMMAND_FAILED, Message: fmt.Sprintf("problem detecting MIME type\n%s\n", err)}
// 			}
// 			driveFile.MimeType = mType.String()
// 			_, err = c.Srv.Files.Create(driveFile).Media(f).Do()
// 		} else {
// 			_, err = c.Srv.Files.Create(driveFile).Do()
// 		}
// 	}

// 	if err != nil {
// 		return &types.APIError{Code: types.CLIENT_FAILED, Message: fmt.Sprintf("%s\n", err)}
// 	}
// 	return nil
// }

// // DeleteFile deletes the file or folder (and all files in it) at the given path
// func (c *GoogleDriveClient) DeleteFile(path string) *types.APIError {

// 	// find the file we want to delete
// 	file, err := c.FindFile(path)
// 	if err != nil {
// 		return &types.APIError{Code: types.FILE_NOT_FOUND, Message: fmt.Sprintf("%s\n", err)}
// 	}

// 	// we cannot delete the root
// 	if file.Id == "root" {
// 		return &types.APIError{Code: types.INVALID_INPUT, Message: "cannot delete root"}
// 	}

// 	err = c.Srv.Files.Delete(file.Id).Do()
// 	if err != nil {
// 		return &types.APIError{Code: types.CLIENT_FAILED, Message: fmt.Sprintf("%s\n", err)}
// 	}
// 	return nil
// }

// // DownloadFile downlaods the file at filePath to the dir specified by downloadPath
// func (c *GoogleDriveClient) DownloadFile(filePath, downloadPath string) *types.APIError {

// 	// find what file to download
// 	file, err := c.FindFile(filePath)
// 	if err != nil {
// 		return &types.APIError{Code: types.FILE_NOT_FOUND, Message: fmt.Sprintf("%s\n", err)}
// 	}

// 	res, err := c.Srv.Files.Get(file.Id).Download()
// 	//res, err := srv.Files.Export(file.Id, file.MimeType).Download()
// 	if err != nil {
// 		return &types.APIError{Code: types.CLIENT_FAILED, Message: fmt.Sprintf("%s\n", err)}
// 	}
// 	defer res.Body.Close()

// 	// make sure we have a slash at the end of our downloadPath
// 	slash := "/"
// 	if downloadPath == "" || string(downloadPath[len(downloadPath)-1]) == "/" {
// 		slash = ""
// 	}

// 	// create file on local machine
// 	localFile, err := os.Create(downloadPath + slash + file.Name)
// 	if err != nil {
// 		return &types.APIError{Code: types.COMMAND_FAILED, Message: fmt.Sprintf("%s\n", err)}
// 	}
// 	defer localFile.Close()

// 	// copy drive file to local file
// 	_, err = io.Copy(localFile, res.Body)
// 	if err != nil {
// 		return &types.APIError{Code: types.COMMAND_FAILED, Message: fmt.Sprintf("%s\n", err)}
// 	}
// 	return nil
// }

// // FindFile find the file/folder given by path
// // Path Format: "dir/.../dir/{file,dir}" or just "{file,dir}" if in Google Drive root
// func (c *GoogleDriveClient) FindFile(path string) (*drive.File, error) {

// 	// return the root if that is what we are looking for
// 	if path == "" || path == "/" {
// 		return &drive.File{Id: "root"}, nil
// 	}

// 	// remove leading / if necessary
// 	if string(path[0]) == "/" {
// 		path = path[1:]
// 	}

// 	// remove trailing / if necessary
// 	if string(path[len(path)-1]) == "/" {
// 		path = path[:len(path)-1]
// 	}

// 	parents := strings.Split(path, "/")
// 	target := parents[len(parents)-1]

// 	// loop through dirs to find where the target resides
// 	parent := "root"
// 	for i := 0; i < len(parents)-1; i++ {

// 		// get the current folder using the previous parent ID
// 		res, err := c.Srv.Files.List().Q(fmt.Sprintf("name='%s' and parents in '%s' and trashed=false", parents[i], parent)).Fields("files(id)").Do()

// 		// Exit if error or no results
// 		if err != nil || (res != nil && res.Files != nil && len(res.Files) == 0) {
// 			return nil, fmt.Errorf("could not get folder '%s'\n%s", parents[i], err)
// 		}
// 		parent = res.Files[0].Id
// 	}

// 	// Get target in the current folder
// 	res, err := c.Srv.Files.List().Q(fmt.Sprintf("name='%s' and parents in '%s' and trashed=false", target, parent)).Fields("files(id, name, size, mimeType)").Do()
// 	if err != nil || (res != nil && res.Files != nil && len(res.Files) == 0) {
// 		return nil, fmt.Errorf("could not get folder/file '%s'\n%s", target, err)
// 	}
// 	return res.Files[0], nil
// }

// func (c *GoogleDriveClient) String() string {
// 	return c.Name
// }
