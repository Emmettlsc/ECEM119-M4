package main

import (
	"flag"
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

	cl := &client{conn: conn, send: make(chan []byte, 256)} // Buffering the send channel
	mu.Lock()
	clients[cl] = true
	mu.Unlock()

	go writePump(cl)

	for {
		_, message, err := conn.ReadMessage()
		if err != nil {
			if websocket.IsUnexpectedCloseError(err, websocket.CloseGoingAway, websocket.CloseAbnormalClosure) {
				log.Printf("error: %v", err)
			}
			break
		}
		broadcast <- message
	}

	mu.Lock()
	delete(clients, cl)
	close(cl.send) // Close the send channel when the client disconnects
	mu.Unlock()
}

func handleMessages() {
	for {
		msg := <-broadcast
		mu.Lock()
		for cl := range clients {
			select {
			case cl.send <- msg:
			default:
				// Avoid closing the channel here; handle it in writePump
			}
		}
		mu.Unlock()
	}
}

func writePump(cl *client) {
	defer cl.conn.Close()
	for {
		msg, ok := <-cl.send
		if !ok {
			cl.conn.WriteMessage(websocket.CloseMessage, []byte{})
			return // Exit the goroutine when send channel is closed
		}
		if err := cl.conn.WriteMessage(websocket.TextMessage, msg); err != nil {
			log.Println("write error:", err)
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
