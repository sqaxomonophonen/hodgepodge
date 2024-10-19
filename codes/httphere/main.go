package main

import "flag"

import "net/http"
import "log"
import "fmt"
import "os"

var bind = ":8080"
var root = "."

func usage(status int) {
	fmt.Fprintf(os.Stderr, "Usage: %s [-h] [-r directory] [bind]\n", os.Args[0])
	flag.PrintDefaults()
	fmt.Fprintf(os.Stderr, "[bind]  Bind address, default is %#v.\n", bind)
	os.Exit(status)
}

type ResponseWriterStats struct {
	responseWriter http.ResponseWriter
	bytesWritten   int64
	statusCode     int
}

func (r *ResponseWriterStats) Header() http.Header {
	return r.responseWriter.Header()
}

func (r *ResponseWriterStats) Write(bs []byte) (int, error) {
	written, err := r.responseWriter.Write(bs)
	r.bytesWritten += int64(written)
	return written, err
}

func (r *ResponseWriterStats) WriteHeader(statusCode int) {
	r.statusCode = statusCode
	r.responseWriter.WriteHeader(statusCode)
}

func (r *ResponseWriterStats) Stats() string {
	status := r.statusCode
	if status == 0 {
		status = 200
	}
	return fmt.Sprintf("%d (b:%d)", status, r.bytesWritten)
}

func main() {
	help := false
	flag.BoolVar(&help, "h", help, "Print usage")
	flag.StringVar(&root, "r", root, "Root directory to serve")
	flag.Parse()

	if help {
		usage(0)
	}

	if flag.NArg() == 1 {
		bind = flag.Args()[0]
	} else if flag.NArg() != 0 {
		usage(1)
	}

	fs := http.FileServer(http.Dir(root))

	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		h := w.Header()
		// these enable "secure-only" features? for instance,
		// SharedArrayBuffer isn't available on http:// URLs unless
		// these headers are sent
		h.Add("Cross-Origin-Embedder-Policy", "require-corp")
		h.Add("Cross-Origin-Opener-Policy", "same-origin")
		rws := &ResponseWriterStats{responseWriter: w}
		fs.ServeHTTP(rws, r)
		log.Printf("%s %s -> %s", r.Method, r.URL.String(), rws.Stats())
	})

	log.Printf("Serving directory %#v at %s", root, bind)

	log.Fatal(http.ListenAndServe(bind, nil))
}
