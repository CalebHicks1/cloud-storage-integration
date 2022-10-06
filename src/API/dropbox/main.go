package main

import (
	"flag"
	"types"

	"github.com/dropbox/dropbox-sdk-go-unofficial/v6/dropbox"
	"github.com/dropbox/dropbox-sdk-go-unofficial/v6/dropbox/users"
)

type DropBoxClient struct {
	types.APIClient
	Srv  *users.Client
	Name string
}

const TOKEN = "sl.BQiZltjI6NylGdMqa430mQZqyMV-dXhrLXMsVnoOox-Oa8wpMWhbeSfsJkohfwXcFKK7JGqpBzFP6ki_aVJC9K3Ni9Nt46mvDz-P901yGJ7JytPlmSey1nW8ngIsSfUYCw3e8OE"

func main() {

	debug := flag.Bool("debug", false, "prints successs statments for debugging")
	flag.Parse()

	config := dropbox.Config{
		Token:    TOKEN,
		LogLevel: dropbox.LogInfo, // if needed, set the desired logging level. Default is off
	}
	dbx := users.New(config)

	api := types.APIClient{Client: &DropBoxClient{Srv: &dbx, Name: "Dropbox"}}
	api.Serve(*debug)
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
