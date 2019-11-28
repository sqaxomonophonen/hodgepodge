package main

import "os"
import "fmt"
import "time"
import "net/http"

func usage() {
	fmt.Fprintf(os.Stderr, "Usage: %s [duration]\n", os.Args[0])
	fmt.Fprintf(os.Stderr, "Prints current GMT time by default, but you can add/subtract a duration, e.g.:\n")
	fmt.Fprintf(os.Stderr, "   %s +30s    will print the time 30 seconds into the future\n", os.Args[0])
	os.Exit(1)
}

func main() {
	var add time.Duration
	argc := len(os.Args)
	if argc == 1 {
		// OK
	} else if argc == 2 {
		var err error
		add, err = time.ParseDuration(os.Args[1])
		if err != nil {
			fmt.Fprintf(os.Stderr, "ERROR: invalid duration argument\n\n")
			usage()
		}
	} else {
		usage()
	}
	fmt.Printf("%s", time.Now().Add(add).UTC().Format(http.TimeFormat))
}
