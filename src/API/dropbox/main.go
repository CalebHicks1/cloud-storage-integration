package main

import (
	"flag"
	"io"
	"os"
	"types"

	"github.com/dropbox/dropbox-sdk-go-unofficial/v6/dropbox"
	"github.com/dropbox/dropbox-sdk-go-unofficial/v6/dropbox/files"
)

type DropBoxClient struct {
	types.APIClient
	Name string
}

const TOKEN = "sl.BRb2-zP_JMm7Q1qEubUVcMCnLHUjztSN0qt4euXt6u1KLnLgSo2wpu6ahBEmqXQdAoeLCGrAbuJckvdQ8KRuyD-WxxmPQ0YyhJUNunpOYZKYiC114rG-WUNfPwDD4gQj1MgOkrZ6CSXq"

func main() {

	debug := flag.Bool("debug", false, "prints successs statments for debugging")
	flag.Parse()

	config := dropbox.Config{
		Token:    TOKEN,
		LogLevel: dropbox.LogInfo, // if needed, set the desired logging level. Default is off
	}
	//dbx := users.New(config)
	/*acc, err := dbx.GetCurrentAccount()
	if err != nil {
		log.Fatal(err)
	}*/

	arg := files.NewDownloadArg("/Main/midterm_links.txt")
	dbx := files.New(config)
	_, contents, err := dbx.Download(arg)
	if err != nil {
		panic(err)
	}

	f, err := os.Create("midterm_links.txt")
	if err != nil {
		panic(err)
	}

	//progressBar := &ioprogress.Reader

	if _, err = io.Copy(f, contents); err != nil {
		panic(err)
	}

	if *debug {
		api := types.APIClient{Client: &DropBoxClient{Name: "Dropbox"}}
		api.Serve(*debug)
	}
}

func (c *DropBoxClient) GetFiles(path string) ([]types.File, *types.APIError) {
	return nil, nil
}

func (c *DropBoxClient) UploadFile(file, path string) *types.APIError {
	return nil
}

func (c *DropBoxClient) DeleteFile(path string) *types.APIError {
	return nil
}

func (c *DropBoxClient) DownloadFile(filePath, downloadPath string) *types.APIError {
	return nil
}

func (c *DropBoxClient) String() string {
	return c.Name
}
