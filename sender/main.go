package main

import (
	"fmt"
)

var socketPath = "/tmp/go_agent.sock"

func main() {
	err, unix := CreateUnix()

	if err != nil {
		fmt.Println("Sender: failed to create Unix socket:", err)
		return
	}

    sender:=CreateSender(unix)

    unix.sender=&sender;

	sender.unix_socket.StartListener()
}





