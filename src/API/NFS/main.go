package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
)

//example command: {"command":"ls", "path":"/", "file":"example"}
type Input struct {
	Command string
	Path    string
	File    string
}

func main() {
	// get executable path for 'mount' command
	mountExecutable, err := exec.LookPath("mount")
	if err != nil {
		fmt.Print(err)
	}
	// `mount` command
	mountCommand := &exec.Cmd{
		Path:   mountExecutable,
		Args:   []string{mountExecutable, "cap.calebhicks.net:/mnt/nfs_shared_dir", "/mnt/nfs_client/"},
		Stdout: os.Stdout,
		Stderr: os.Stdout,
	}
	umountExecutable, err := exec.LookPath("umount")
	if err != nil {
		fmt.Print(err)
	}

	// `umount` command
	umountCommand := &exec.Cmd{
		Path:   umountExecutable,
		Args:   []string{umountExecutable, "/mnt/nfs_client/"},
		Stdout: os.Stdout,
		Stderr: os.Stdout,
	}

	// mount nfs drive
	if err := mountCommand.Run(); err != nil {
		fmt.Println("Error: ", err)
	}

	reader := bufio.NewReader(os.Stdin)
	read_loop := true

	for read_loop {
		text, _ := reader.ReadString('\n')
		var input Input // input command
		json.Unmarshal([]byte(text), &input)

		fmt.Println(input)
		fmt.Println(input.Path, input.Command, input.File)
		read_loop = false
	}

	// unmount nfs drive
	if err := umountCommand.Run(); err != nil {
		fmt.Println("Error: ", err)
	}
}
