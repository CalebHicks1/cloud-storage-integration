package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"types"
	"io"
)

func main() {
	mountPath := "/mnt/nfs_client/"
	nfsServerPath := "cap.calebhicks.net:/mnt/nfs_shared_dir"
	
	// mount nfs drive
	err := mountNFS(mountPath, nfsServerPath)
	if err != nil {
		fmt.Println("Error", err)
		
	}

	reader := bufio.NewReader(os.Stdin)
	read_loop := true

	for read_loop {
		text, err := reader.ReadString('\n')
		if err != nil {
			if err == io.EOF {
				fmt.Println("Error: ", err)
				read_loop=false
				break; // leave loop when file descriptor closes
			}else {
				fmt.Println("Error: ", err)
				read_loop=false
				break;
			}	
		}
		// parse command
		parseJsonInput(text, mountPath)
	}

	// unmount nfs drive
	// err = umountNFS(mountPath)
	// if err != nil {
	// 	fmt.Println("Error", err)
	// }
}

func mountNFS(mountPath string, nfsServerPath string) (err error) {
	// get path for mount executable
	mountExecutable, err := exec.LookPath("mount")
	if err != nil {
		fmt.Print(err)
	}
	// `mount` command
	mountCommand := &exec.Cmd{
		Path:   mountExecutable,
		Args:   []string{mountExecutable, nfsServerPath, mountPath},
		Stdout: os.Stdout,
		Stderr: os.Stdout,
	}
	if err := mountCommand.Run(); err != nil {
		return err
	}
	return nil
}

func umountNFS(mountPath string) (err error) {
	umountExecutable, err := exec.LookPath("umount")
	if err != nil {
		fmt.Print(err)
	}

	// `umount` command
	umountCommand := &exec.Cmd{
		Path:   umountExecutable,
		Args:   []string{umountExecutable, mountPath},
		Stdout: os.Stdout,
		Stderr: os.Stdout,
	}
	// execute umount
	if err := umountCommand.Run(); err != nil {
		fmt.Println("Error: ", err)
	}
	return nil
}

// FILE OPERATIONS ///////////////////////////////////////////////////

/*
	Read the command argument from json input, then print the relevant response.
*/
func parseJsonInput(text string, mountPath string) {
	var cmd types.Command // input command
	json.Unmarshal([]byte(text), &cmd)

	var response string

	switch cmd.Type {
	case "list":
		response = list(cmd.Path, mountPath)
	case "shutdown":
		response = "{\"info\":\"shutting down\"}"
		umountNFS(mountPath)
		os.Exit(0);
	default:
		response = "{\"error\": \"command not implemented\"}"
	}
	fmt.Println(response)
}

/*
	path: the path given by the fuse call
	mountPath: the path that the nfs server is mounted to.
	Return json string that will be printed
*/
func list(path string, mountPath string) string {

	//clean up path, assuming mountPath ends with /
	if path == "/" || path == "." {
		path = ""
	} else if len(path) > 0 && path[0] == '/' {
		path = path[1:]
	}

	absolutePath := mountPath + path
	fileSlice := readDir(absolutePath)

	// build json
	jsonFileSlice, err := json.Marshal(fileSlice)
	if err != nil {
		fmt.Println("Error: ", err)
	}

	response := string(jsonFileSlice)
	return response
}

/*
	Reads the directory at the given path (relative to the mounted nfs drive)
	source: https://golang.cafe/blog/how-to-list-files-in-a-directory-in-go.html
	Returns a list of file objects
*/
func readDir(path string) []types.File {
	// get list of files
	var fileSlice []types.File
	files, err := ioutil.ReadDir(path)
	if err != nil {
		fmt.Println("Error: ", err)
	}

	for _, file := range files {
		//create file struct
		newFile := types.File{
			Name:  file.Name(),
			IsDir: file.IsDir(),
			Size:  file.Size(),
		}
		fileSlice = append(fileSlice, newFile)
	}
	return fileSlice
}
