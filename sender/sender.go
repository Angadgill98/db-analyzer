package main

import (
	"encoding/json"
	"fmt"
	"net/http"
)


type Sender struct{
	client http.Client
	unix_socket Unix_Socket
	routines []chan []byte
}

var routines_count =3 //here 

func CreateSender(unix Unix_Socket)Sender{
	sender:= Sender{
		client: http.Client{},
		unix_socket:unix,
		routines: []chan []byte{},
	}
	for i := 1; i <= routines_count; i++ {
		channel := make(chan []byte, 100)

		sender.routines = append(sender.routines, channel)

		go HandleRoutine(channel)
	}


	return sender;
}



func HandleRoutine(channel <-chan []byte) {
	for payload := range channel {
		fmt.Println("Sender: Received:", string(payload))


		operation, err :=ParseOperation(payload)
		if err != nil {
			fmt.Println("Sender: failed to parse operation:", err)
			return
		}

		fmt.Printf("Sender:Complete json is %+v\n", operation)
	}
}

func ParseOperation(payload []byte) (any, error) {
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

		return operation, nil

	case "JOIN_ROOM", "CREATE_ROOM":
		var operation BackendCommandOperation

		err := json.Unmarshal(payload, &operation)
		if err != nil {
			return nil, err
		}

		return operation, nil

	default:
		return nil, fmt.Errorf("unknown operation: %s", header.OperationName)
	}
}
