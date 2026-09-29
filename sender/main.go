package main

import (
	"fmt"

	"github.com/joho/godotenv"
)

var socketPath = "/tmp/go_agent.sock"

func main() {

    err := godotenv.Load()
    if err != nil {
        fmt.Println("Failed to load .env:", err)
        return
    }
	err, unix := CreateUnix()

	if err != nil {
		fmt.Println("Sender: failed to create Unix socket:", err)
		return
	}

    sender:=CreateSender(&unix)

    unix.sender=&sender;

	sender.unix_socket.StartListener()
}





