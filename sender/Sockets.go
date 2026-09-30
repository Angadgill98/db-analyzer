package main

import (
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"os"
)


type Unix_Socket struct{
	listener net.Listener
	sender   *Sender
}

func CreateUnix()(error,Unix_Socket){
	os.Remove(socketPath)

	listener, err := net.Listen("unix", socketPath)

	if err != nil {
		fmt.Println("Sender: listen failed:", err)
		return err, Unix_Socket{}
	}

	return nil,Unix_Socket{
		listener: listener,
		sender: &Sender{},
	}
}



func (unix Unix_Socket) StartListener() {
	for {
		conn, err := unix.listener.Accept()

		if err != nil {
			fmt.Println("Sender: accept failed:", err)
			continue
		}

		fmt.Println("Sender: collector connected")

		go unix.handleConnection(conn)
	}
}


func (unix *Unix_Socket) handleConnection(conn net.Conn) {
	defer conn.Close()
	var currentRoutine=0;
	for {
		lengthBuffer := make([]byte, 8)

		_, err := io.ReadFull(conn, lengthBuffer)
		if err != nil {
			fmt.Println("Sender: failed to read length:", err)
			return
		}

		length := binary.LittleEndian.Uint64(lengthBuffer)

		payload := make([]byte, length)

		_, err = io.ReadFull(conn, payload)
		if err != nil {
			fmt.Println("Sender: failed to read payload:", err)
			return
		}
		fmt.Printf("Sender: received %d bytes\n", len(payload))
		// fmt.Println("Sender: received:", string(payload))


		


		unix.sender.routines[currentRoutine] <- payload

		currentRoutine++

		if currentRoutine >= len(unix.sender.routines) {
			currentRoutine = 0
		}
	}
}


