package main

import (
	"bufio"
	"fmt"
	"io"
	"os/exec"
)

func main() {
	cmd := exec.Command("./google_drive_client")
	//cmd := exec.Command("cat")

	o, _ := cmd.StdoutPipe()
	i, _ := cmd.StdinPipe()

	defer o.Close()
	defer i.Close()

	//cmd.Stdout = os.Stdout
	//cmd.Stderr = os.Stderr

	cmd.Start()

	reader := bufio.NewReader(o)
	//writer := bufio.NewWriter(i)
	//yer := make([]byte, 0)
	//_, _ = o.Read(yer)

	//yer, _ := reader.ReadString('\n')
	//fmt.Println(yer)
	for j := 0; j < 3; j++ {

		//writer.WriteString(`{"command":"list","path":"","file":""}\n`)
		io.WriteString(i, "{\"command\":\"list\",\"path\":\"\",\"file\":\"\"}\n")
		//io.WriteString(i, `main.go`)

		fmt.Println("wrote to fd")

		//_, _ = i.Write([]byte(`{"command":"list","path":"","file":""}\n`))
		//o.Read(yer)

		yer, _ := reader.ReadString('\n')

		fmt.Println(string(yer))
	}

	io.WriteString(i, "{\"command\":\"shutdown\",\"path\":\"\",\"file\":\"\"}\n")
	yer, _ := reader.ReadString('\n')

	fmt.Println(string(yer))
}
