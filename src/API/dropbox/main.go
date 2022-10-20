package main

import (
	"flag"
	"fmt"
	"io"
	"os"
	"types"

	"github.com/dropbox/dropbox-sdk-go-unofficial/v6/dropbox"
	"github.com/dropbox/dropbox-sdk-go-unofficial/v6/dropbox/files"
)

type DropBoxClient struct {
	types.APIClient
	Srv  files.Client
	Name string
}

// TODO: switch to using OAuth2.0 (token expires very quickly)
const TOKEN = "sl.BRaKUjPmUUcf7RuJoQmCnNbGi03CzevVOAAgK1cyh_fMbKI88dEDYWNj-L5kxQU9DMAULoxlm5UYfrMK23djUf-3Gk2RBpuDSH7vtikFgXJSVQR2NEOD0EvZh7ufgq81ucrJ6kymh2Ov"

func main() {

	debug := flag.Bool("debug", false, "prints successs statments for debugging")
	flag.Parse()

	config := dropbox.Config{
		Token:    TOKEN,
		LogLevel: dropbox.LogInfo, // if needed, set the desired logging level. Default is off
	}
	dbx := files.New(config)

	api := types.APIClient{Client: &DropBoxClient{Name: "Dropbox", Srv: dbx}}
	api.Serve(*debug)
}

func (c *DropBoxClient) GetFiles(path string) ([]types.File, *types.APIError) {

	// if not blank, add leading "/" and remove trailing "/"
	if path != "" {
		path = c.formatPath(path, true, false, false, true)
	}

	// get the entries in the given directory
	res, err := c.Srv.ListFolder(files.NewListFolderArg(path))
	if err != nil {
		return nil, &types.APIError{Code: types.CLIENT_FAILED, Message: fmt.Sprintf("%s\n", err)}
	}

	// loop through all files and folders in the directory
	var ret []types.File
	for _, entry := range res.Entries {
		switch f := entry.(type) {
		case *files.FileMetadata:
			ret = append(ret, types.File{Name: f.Name, IsDir: false, Size: f.Size})
		case *files.FolderMetadata:
			ret = append(ret, types.File{Name: f.Name, IsDir: true, Size: 0})
		}
	}

	return ret, nil
}

// UploadFile uploads the given file to the given path in the drive
func (c *DropBoxClient) UploadFile(file, path string) *types.APIError {

	// open our file
	f, err := os.Open(file)
	if err != nil {
		return &types.APIError{Code: types.COMMAND_FAILED, Message: fmt.Sprintf("problem openeing\n%s\n", err)}
	}
	defer f.Close()
	fileInfo, err := f.Stat()
	if err != nil {
		return &types.APIError{Code: types.COMMAND_FAILED, Message: fmt.Sprintf("problem statting\n%s\n", err)}
	}

	// upload and overwrite if it exists (leading and trailing / needed)
	uplpoadArg := files.NewUploadArg(c.formatPath(path, true, true, false, false) + fileInfo.Name())
	uplpoadArg.Mode.Tag = "overwrite"
	if _, err = c.Srv.Upload(uplpoadArg, f); err != nil {
		return &types.APIError{Code: types.CLIENT_FAILED, Message: fmt.Sprintf("%s\n", err)}
	}
	return nil
}

// DeleteFile deletes the file or folder (and all files in it) at the given path
func (c *DropBoxClient) DeleteFile(path string) *types.APIError {

	// attempt to delete file (leading / needed, remove trailing /)
	if _, err := c.Srv.DeleteV2(files.NewDeleteArg(c.formatPath(path, true, false, false, true))); err != nil {
		return &types.APIError{Code: types.CLIENT_FAILED, Message: fmt.Sprintf("%s\n", err)}
	}
	return nil
}

func (c *DropBoxClient) DownloadFile(filePath, downloadPath string) *types.APIError {

	// download the file from Dropbox (leading / needed)
	res, contents, err := c.Srv.Download(files.NewDownloadArg(c.formatPath(filePath, true, false, false, false)))
	if err != nil {
		return &types.APIError{Code: types.CLIENT_FAILED, Message: fmt.Sprintf("%s\n", err)}
	}
	defer contents.Close()

	// create file on local machine (trailing / needed)
	f, err := os.Create(c.formatPath(downloadPath, false, true, false, false) + res.Name)
	if err != nil {
		return &types.APIError{Code: types.COMMAND_FAILED, Message: fmt.Sprintf("%s\n", err)}
	}

	// copy dropbox file to local file
	if _, err = io.Copy(f, contents); err != nil {
		return &types.APIError{Code: types.COMMAND_FAILED, Message: fmt.Sprintf("%s\n", err)}
	}
	return nil
}

// FormatPath adds and removes trailing and leading forward slashes to the path
func (c *DropBoxClient) formatPath(path string, addLeading, addTrailing, removeLeading, removeTrailing bool) string {

	if addLeading && (path == "" || path[0] != '/') {
		path = "/" + path
	} else if removeLeading && path != "" && path[0] == '/' {
		path = path[1:]
	}

	if addTrailing && (path == "" || path[len(path)-1] != '/') {
		path += "/"
	} else if removeTrailing && path != "" && path[len(path)-1] == '/' {
		path = path[:len(path)-1]
	}

	return path
}

func (c *DropBoxClient) String() string {
	return c.Name
}
