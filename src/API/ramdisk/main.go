package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io/fs"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path"
	"strings"
)

const ConfigPath = "config.json"

var config Config
var isMounted = false

// https://www.sohamkamani.com/golang/enums/
// There will be a lot of links since I don't know Go well -BG
type ErrorCode int32

const (
	ClientFailed   ErrorCode = 1
	CommandFailed  ErrorCode = 2
	InvalidCommand ErrorCode = 3
	InvalidInput   ErrorCode = 4
)

type Config struct {
	Name      string
	Type      string
	MountPath string
	Size      string
}

// From Caleb's API branch
type Command struct {
	Command string
	Path    string
	File    string
}

type File struct {
	Name  string
	IsDir bool
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

	log.Fatal(err)
}

// ...loads the configuration
func loadConfig(path string) Config {
	// https://zetcode.com/golang/readfile/
	content, err := ioutil.ReadFile(path)
	if err != nil {
		fatal(err)
	}

	// https://www.sohamkamani.com/golang/json/
	var config Config
	err = json.Unmarshal([]byte(content), &config)
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

func unmount() {
	cmd := exec.Command("umount", config.MountPath)
	err := cmd.Run()
	if err != nil {
		fatal(err)
	}

	isMounted = false
}

// Listens for API calls on stdin
func listen() {
	reader := bufio.NewReader(os.Stdin)

	shutdown := false
	for !shutdown {
		// https://stackoverflow.com/questions/20895552/how-can-i-read-from-standard-input-in-the-console
		input, err := reader.ReadString('\n')

		shutdown = processInput(input, err)
	}
}

// Processes input, returning true when shutdown is called, false otherwise
func processInput(input string, err error) bool {
	if err != nil {
		reportError(ClientFailed)
		// TODO: There's a lot of return falses, refactor this?
		return false
	}

	var command Command
	err = json.Unmarshal([]byte(input), &command)
	if err != nil {
		reportError(InvalidInput)
		return false
	}

	switch command.Command {
	case "list":
		var entries []fs.DirEntry

		path := path.Join(config.MountPath, command.Path)
		entries, err = os.ReadDir(path)
		if err != nil {
			reportError(CommandFailed)
			return false
		}

		var filenames []string
		for _, entry := range entries {
			filenames = append(filenames, entry.Name())
		}

		msg := fmt.Sprintf("{\"%s\"}", strings.Join(filenames, "\", \""))
		fmt.Print(msg)
	case "upload":
		path := path.Join(config.MountPath, command.Path)
		cmd := exec.Command("cp", command.File, path)

		err = cmd.Run()
		if err != nil {
			reportError(CommandFailed)
			return false
		}

		reportSuccess()
	case "delete":
		path := path.Join(config.MountPath, command.Path)
		cmd := exec.Command("rm", path)

		err = cmd.Run()
		if err != nil {
			reportError(CommandFailed)
			return false
		}

		reportSuccess()
	case "shutdown":
		reportSuccess()
		return true
	default:
		reportError(InvalidCommand)
	}

	return false
}

// Report an error to the parent program
func reportError(code ErrorCode) {
	msg := fmt.Sprintf("{\"%d\"}", code)
	fmt.Print(msg)
}

// Report a success to the parent program
func reportSuccess() {
	fmt.Print("{\"SUCCESS\"}")
}
