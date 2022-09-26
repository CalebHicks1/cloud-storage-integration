package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"io/fs"
	"os"
	"os/exec"
	"path"
	"types"
)

const ConfigPath = "config.json"

var config Config
var isMounted = false

type Config struct {
	Name      string
	Type      string
	MountPath string
	Size      string
}

func main() {
	// TODO: Turn ConfigPath constant into argument?
	config = loadConfig(ConfigPath)

	mount()
	listen()
	unmount()
}

// Tries to unmount before exiting
func fatal(err error) {
	if isMounted {
		unmount()
	}

	// This reports a client error as specified by the API
	fmt.Printf("{%d}\n", types.CLIENT_FAILED)
	os.Exit(1)
}

// ...loads the configuration
func loadConfig(path string) Config {
	configFile, err := os.Open(path)
	if err != nil {
		fatal(err)
	}

	configData, err := io.ReadAll(configFile)
	if err != nil {
		fatal(err)
	}

	// https://www.sohamkamani.com/golang/json/
	var config Config
	err = json.Unmarshal(configData, &config)
	if err != nil {
		fatal(err)
	}

	return config
}

// Mounts the ramdisk
func mount() {
	// Make sure config.MountPath exists to mount to
	// https://freshman.tech/snippets/go/check-file-exists/
	_, err := os.Stat(config.MountPath)
	if err != nil {
		// If it doesn't exist then make it
		// https://gosamples.dev/create-directory/
		if os.IsNotExist(err) {
			err = os.MkdirAll(config.MountPath, os.ModePerm)
		}

		// err can be nil here from MkdirAll
		if err != nil {
			fatal(err)
		}
	}

	// mount -o size=<size> -t <type> <name> <mount path>
	// https://unix.stackexchange.com/questions/66329/creating-a-ram-disk-on-linux
	// https://stackoverflow.com/questions/6182369/exec-a-shell-command-in-go
	size := fmt.Sprintf("size=%s", config.Size)
	cmd := exec.Command("mount", "-o", size, "-t", config.Type, config.Name, config.MountPath)

	err = cmd.Run()
	if err != nil {
		fatal(err)
	}

	isMounted = true
}

// Unmounts the ramdisk
func unmount() {
	// isMounted is false here because otherwise the call to fatal()
	// would cause an infinite loop on command failure
	isMounted = false

	cmd := exec.Command("umount", config.MountPath)
	err := cmd.Run()
	if err != nil {
		fatal(err)
	}
}

// Listens for API calls on stdin. Continues infinitely until a shutdown
// command is received or a fatal error occurs
func listen() {
	reader := bufio.NewReader(os.Stdin)

	shutdown := false
	for !shutdown {
		// https://stackoverflow.com/questions/20895552/how-can-i-read-from-standard-input-in-the-console
		input, err := reader.ReadString('\n')

		if err != nil {
			fatal(err)
		}

		shutdown = processInput(input)
	}
}

// Processes input, returning true when shutdown is called, false otherwise
func processInput(input string) bool {
	response := success()
	shutdown := false

	var cmd types.Command
	err := json.Unmarshal([]byte(input), &cmd)
	if err != nil {
		response = errorCode(types.INVALID_INPUT)
	}

	if response.ErrCode == types.NO_ERROR {
		switch cmd.Type {
		case "list":
			response = list(cmd)
		case "upload":
			response = upload(cmd)
		case "delete":
			response = delete(cmd)
		case "shutdown":
			shutdown = true
		default:
			response = errorCode(types.INVALID_COMMAND)
		}
	}

	if response.Files != nil || response.ErrCode != types.NO_ERROR {
		fmt.Println(response)
	}

	return shutdown
}

// Executes the list command according to the API spec
func list(cmd types.Command) types.Response {
	path := path.Join(config.MountPath, cmd.Path)
	entries, err := os.ReadDir(path)
	if err != nil {
		return errorCode(types.COMMAND_FAILED)
	}

	var files []types.File
	for _, entry := range entries {
		var info fs.FileInfo
		info, err = entry.Info()
		if err != nil {
			return errorCode(types.COMMAND_FAILED)
		}

		file := types.File{
			Name:  info.Name(),
			IsDir: info.IsDir(),
			Size:  info.Size(),
		}
		files = append(files, file)
	}

	return types.Response{
		ErrCode: types.NO_ERROR,
		Files:   files,
	}
}

// Executes the upload command according to the API spec
func upload(cmd types.Command) types.Response {
	// https://opensource.com/article/18/6/copying-files-go
	srcPath := cmd.File
	destPath := path.Join(config.MountPath, cmd.Path)

	// Make sure src exists and is a regular file
	srcStat, err := os.Stat(srcPath)
	if err != nil || !srcStat.Mode().IsRegular() {
		return errorCode(types.COMMAND_FAILED)
	}

	// Open source
	src, err := os.Open(srcPath)
	if err != nil {
		return errorCode(types.COMMAND_FAILED)
	}
	defer src.Close()

	// Create destination
	dest, err := os.Create(destPath)
	if err != nil {
		return errorCode(types.COMMAND_FAILED)
	}
	defer dest.Close()

	// Copy
	_, err = io.Copy(dest, src)
	if err != nil {
		return errorCode(types.COMMAND_FAILED)
	}

	return success()
}

// Executes the delete command according to the API spec
func delete(cmd types.Command) types.Response {
	path := path.Join(config.MountPath, cmd.Path)
	err := os.RemoveAll(path)
	if err != nil {
		return errorCode(types.COMMAND_FAILED)
	}

	return success()
}

// Returns a response struct set with the corresponding ErrorCode
func errorCode(code types.ErrorCode) types.Response {
	return types.Response{
		ErrCode: code,
		Files:   nil,
	}
}

// Returns a response struct with ErrorCode NO_ERROR
func success() types.Response {
	return types.Response{
		ErrCode: types.NO_ERROR,
	}
}
