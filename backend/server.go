package main

import (
	"flag"
	"fmt"
	"github.com/gorilla/websocket"
	"log"
	"net/http"
	"sync"
)

var addr = flag.String("addr", "0.0.0.0:8080", "http service address")

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool {
		return true // Allow all origins
	},
}

// client represents a connected WebSocket client.
type client struct {
	conn *websocket.Conn
	send chan []byte
}

var (
	clients   = make(map[*client]bool) // Connected clients
	broadcast = make(chan []byte)      // Broadcast channel
	mu        sync.Mutex               // Mutex for clients
)

func echo(w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Print("upgrade:", err)
		return
	}
	defer conn.Close()

	cl := &client{conn: conn, send: make(chan []byte)}

	mu.Lock()
	clients[cl] = true
	mu.Unlock()

	go writePump(cl) // Start the writePump goroutine for this client

	for {
		_, message, err := conn.ReadMessage()
		if err != nil {
			log.Println("read:", err)
			break
		}
		log.Printf("recv: %s", message)

		// Broadcast the message to all clients
		broadcast <- message
	}

	mu.Lock()
	delete(clients, cl)
	mu.Unlock()
}

func handleMessages() {
	for {
		msg := <-broadcast
		fmt.Printf("Broadcasting message: %s\n", string(msg))

		mu.Lock()
		for cl := range clients {
			// Use a local variable to avoid blocking while holding the mutex
			sendChan := cl.send
			mu.Unlock()

			select {
			case sendChan <- msg:
			default:
				close(cl.send)
				mu.Lock()
				delete(clients, cl)
				mu.Unlock()
			}

			mu.Lock()
		}
		mu.Unlock()
	}
}

func writePump(cl *client) {
	defer func() {
		cl.conn.Close()
		close(cl.send) // Ensure the send channel is closed
	}()

	for {
		msg, ok := <-cl.send
		if !ok {
			// The send channel is closed
			cl.conn.WriteMessage(websocket.CloseMessage, []byte{})
			return
		}
		if err := cl.conn.WriteMessage(websocket.TextMessage, msg); err != nil {
			log.Println("write:", err)
			return
		}
	}
}

func main() {
	flag.Parse()
	log.SetFlags(0)
	http.HandleFunc("/echo", echo)
	go handleMessages()

	// Serve static files from the client directory
	fs := http.FileServer(http.Dir("../client"))
	http.Handle("/", fs)

	log.Fatal(http.ListenAndServe(*addr, nil))
}
