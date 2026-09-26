package main

import (
	"fmt"
	"net"
	"net/http"
	"os"
)

import (
    "encoding/binary"
    "io"
)

var socketPath = "/tmp/go_agent.sock"

func main(){
	err,listener:=CreateUnix();

	if err != nil {
        fmt.Println("failed to create Unix socket:", err)
        return
    }
 

	StartListener(listener);

}




func ConnectToBackend()(http.Client){
	client := http.Client{}
	


	return client;
}

func CreateUnix()(error,net.Listener){
	os.Remove(socketPath)

	listener, err := net.Listen("unix", socketPath)

    if err != nil {
        fmt.Println("Sender: listen failed:", err)
        return err,nil
    }

	return nil,listener
}

func StartListener(listener net.Listener){
	for{
		conn, err := listener.Accept()

        if err != nil {
            fmt.Println("accept failed:", err)
            continue
        }

        fmt.Println("collector connected")

		go handleConnection(conn)
	}
}


func handleConnection(conn net.Conn) {
    defer conn.Close()

    for {
        // Read the first 8 bytes
        lengthBuffer := make([]byte, 8)

        _, err := io.ReadFull(conn, lengthBuffer)
        if err != nil {
            fmt.Println("failed to read length:", err)
            return
        }

        // Convert the 8 bytes into a length
        length := binary.LittleEndian.Uint64(lengthBuffer)

        // Create buffer of exactly that size
        payload := make([]byte, length)

        // Read exactly length bytes
        _, err = io.ReadFull(conn, payload)
        if err != nil {
            fmt.Println("failed to read payload:", err)
            return
        }

        fmt.Println("received:", string(payload))
    }
}

