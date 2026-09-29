package main

import (
	"encoding/json"
	"fmt"
	"net/http"
	"os"
	"time"

	socketio "github.com/Joaquimborges/go-socket.io"
)


type Sender struct{
	client http.Client
	unix_socket *Unix_Socket
	routines []chan []byte
	url string
	rooms_id map[string]struct{}
	socket      *socketio.Client
}

var routines_count =3 //here 

func CreateSender(unix *Unix_Socket)Sender{
	sender:= Sender{
		client: http.Client{},
		unix_socket:unix,
		routines: []chan []byte{},
		url:os.Getenv("BACKEND_URL"),
		rooms_id: make(map[string]struct{}),
	}

	socket, err := ConnectToBackend(sender.url)
    if err != nil {
        fmt.Println("Sender: backend connection failed:", err)
    } else {
	    fmt.Println("Sender: connected to backend")
        sender.socket = socket
    }


	for i := 0; i < routines_count; i++ {
		channel := make(chan []byte, 100)

		sender.routines = append(sender.routines, channel)

		go sender.HandleRoutine(channel)
	}


	return sender;
}


func ConnectToBackend(url string) (*socketio.Client, error) {
 
    fmt.Println("Sender: backend URL:", url)

    socket, err := socketio.NewClient(url)
    if err != nil {
        fmt.Println("Sender: Failed to create Socket Clietn obj:", err)
        return nil, err
    }
	socket.OnConnect(func() {
        fmt.Println("Sender: Socket.IO connection established")
    })

    socket.OnDisconnect(func(err error) {
        fmt.Println("Sender: Socket.IO disconnected:", err)
    })

    socket.OnReconnectAttempt(func(attempt int, backoff time.Duration, err error) {
        fmt.Println("Sender: reconnect attempt:", attempt, "after", backoff, "error:", err)
    })

    err = socket.Connect()
    if err != nil {
        fmt.Println("Sender: backend connection failed:", err)
        return nil, err
    }
    return socket, nil
}


func (s *Sender) HandleRoutine(channel <-chan []byte) {
	for payload := range channel {


		_, err :=s.ParseOperation(payload)
		if err != nil {
			fmt.Println("Sender: failed to parse operation:", err)
			return
		}

	}
}

func (s *Sender) ParseOperation(payload []byte) (any, error) {
	var header OperationHeader

	err := json.Unmarshal(payload, &header)
	if err != nil {
		return nil, err
	}

	switch header.OperationName {
	case "ExecutorStart":
		var operation ExecutorStartOperation

		err := json.Unmarshal(payload, &operation)
		if err != nil {
			return nil, err
		}


		return operation, nil

	case "ExecutorEnd":
		var operation ExecutorEndOperation

		err := json.Unmarshal(payload, &operation)
		if err != nil {
			return nil, err
		}
		s.HandleExecutorEnd(operation)

		return operation, nil

	case "JOIN_ROOM", "CREATE_ROOM":
		var operation BackendCommandOperation


		err := json.Unmarshal(payload, &operation)
		if err != nil {
			return nil, err
		}

		s.HandleBackendCommand(operation)
		fmt.Printf("Sender: BackendCommandOperation = %+v\n", operation)



		return operation, nil

	default:
		return nil, fmt.Errorf("unknown operation: %s", header.OperationName)
	}
}


func (s *Sender) HandleExecutorStart(operation ExecutorStartOperation) {
	// handle ExecutorStart
}


func (s *Sender) HandleExecutorEnd(operation ExecutorEndOperation) {
	if s.socket == nil {
        fmt.Println("Sender: backend socket is not connected")
        return
    }

    if len(s.rooms_id) == 0 {
        fmt.Println("Sender: no rooms joined yet")
        return
    }

    for roomID := range s.rooms_id {
        s.socket.Emit("metrics", map[string]any{
            "room_id": roomID,
            "metrics": operation,
        })
    }
}


func (s *Sender) HandleBackendCommand(operation BackendCommandOperation) {
	roomID := operation.RoomID
	if _, exists := s.rooms_id[roomID]; exists {
		fmt.Println("Room already exists:", roomID)
		return
	}

	fmt.Println("Room does not exist and adding it tot the set: ", roomID)

	s.rooms_id[roomID] = struct{}{}


}