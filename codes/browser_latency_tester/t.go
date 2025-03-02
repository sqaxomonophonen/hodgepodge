package main

import (
	"fmt"
	"github.com/gorilla/websocket"
	"log"
	"net/http"
	"strconv"
)

func main() {
	upgrader := websocket.Upgrader{}

	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		w.Write([]byte(`<!doctype HTML>
		<html>
		<head>
		<style>
		button {
			touch-action: manipulation; /* prevents 300ms "touch delay" on mobile */
		}
		#dt {
			color: gray;
			font-size: 0.7em;
		}
		</style>
		<script>
		window.onload = () => {
			let t0;
			const ws = new WebSocket("ws://" + window.location.host + "/ws");
			ws.addEventListener("open", () => {
				console.log("WS open?");
			});

			const update = (t) => {
				num.innerText=t;
				dt.innerText=Date.now()-t0 + " ms";
			};

			ws.addEventListener("message", (event) => {
				update(event.data);
			})

			btn.addEventListener("click", () => {
				t0 = Date.now();
				fetch("/inc?v="+num.innerText).then(x => {
					x.text().then(x => {
						update(x);
					})
				})
			});

			btnws.addEventListener("click", () => {
				t0 = Date.now();
				ws.send(num.innerText)
			});
		};
		</script>
		</head>
		<body>
		<button id="btn">INC/http</button>
		<button id="btnws">INC/ws</button>
		<span id="num">0</span>
		<span id="dt"></span>
		</body>
		</html>
		`))
		log.Printf("index.html")
	})

	http.HandleFunc("/inc", func(w http.ResponseWriter, r *http.Request) {
		q := r.URL.Query()
		v, err := strconv.Atoi(q.Get("v"))
		if err != nil {
			panic(err)
		}
		fmt.Fprintf(w, "%d", v+1)
		log.Printf("INC")
	})

	http.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		c, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			panic(err)
		}
		defer c.Close()
		for {
			mt, message, err := c.ReadMessage()
			if err != nil {
				panic(err)
			}
			v, err := strconv.Atoi(string(message))
			if err != nil {
				panic(err)
			}
			log.Printf("INC/ws")
			c.WriteMessage(mt, []byte(fmt.Sprintf("%d", v+1)))
		}
	})

	log.Printf("Serving :6510 ...")
	log.Fatal(http.ListenAndServe(":6510", nil))
}
