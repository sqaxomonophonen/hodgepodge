package main

import (
	"context"
	"encoding/json"
	"fmt"
	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/service/s3"
	"html/template"
	"os"
	"strings"
)

const interchangeJSONPath = "fetch.json"

type IXSong struct {
	Key  string
	Size int64
}

func GoFetch() {
	ctx := context.TODO() // lol
	region := "eu-north-1"
	cfg, err := config.LoadDefaultConfig(ctx, config.WithRegion(region))
	if err != nil {
		panic(err)
	}
	s3c := s3.NewFromConfig(cfg)

	paginator := s3.NewListObjectsV2Paginator(s3c, &s3.ListObjectsV2Input{
		Bucket: aws.String("veralin.dk"),
		Prefix: aws.String("music/"),
	})

	list := []IXSong{}

	for paginator.HasMorePages() {
		page, err := paginator.NextPage(ctx)
		if err != nil {
			panic(err)
		}
		for _, obj := range page.Contents {
			list = append(list, IXSong{
				Key:  *obj.Key,
				Size: obj.Size,
			})
		}
	}

	f, err := os.OpenFile(interchangeJSONPath, os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		panic(err)
	}
	defer f.Close()
	enc := json.NewEncoder(f)
	enc.SetIndent("", "\t")
	err = enc.Encode(list)
	if err != nil {
		panic(err)
	}
}

func GoBuild() {
	f, err := os.Open(interchangeJSONPath)
	if err != nil {
		panic(err)
	}

	var list []IXSong
	err = json.NewDecoder(f).Decode(&list)
	if err != nil {
		panic(err)
	}

	f.Close()

	tmpl, err := template.New("template").Parse(`
{{range .}}
<div class="song">
<a class="songlink" href="{{.Href}}" id="{{.Id}}">{{.Label}}</a>
</div>
{{end}}
	`)
	if err != nil {
		panic(err)
	}

	idFromKey := func(key string) string {
		isValidInFragment := func(r rune) bool {
			return ('0' <= r && r <= '9') ||
				('a' <= r && r <= 'z') ||
				('A' <= r && r <= 'Z') ||
				strings.ContainsRune("?:@-._~!$&'()*+,;=", r)
		}

		var ret string
		for _, ch := range key {
			if isValidInFragment(ch) {
				ret += string(ch)
			} else if ch == ' ' {
				ret += "_"
			} else if ch == '#' {
				ret += "*"
			} else if ch == 'Ø' {
				ret += "OE"
			} else if ch == 'ø' {
				ret += "oe"
			} else if ch == 'æ' {
				ret += "ae"
			} else if ch == 'ß' {
				ret += "ss"
			} else if ch == 'å' {
				ret += "aa"
			} else if ch == '½' {
				ret += "1/2"
			} else if ch == '/' {
				ret += "__"
			} else {
				panic(fmt.Sprintf("unhandled rune '%c'", ch))
			}
		}

		return ret
	}

	labelFromKey := func(key string) string {
		xs := strings.Split(key, "/")
		xs = xs[1:len(xs)]
		return strings.Join(xs, "/")
	}

	type Element struct {
		Href  string
		Id    string
		Label string
	}
	var elements []Element
	idSet := map[string]bool{}
	for _, x := range list {
		id := idFromKey(labelFromKey(x.Key)) // ahem
		if idSet[id] {
			fmt.Printf("WARNING: skipping duplicate id %#v\n", id)
			continue
		}
		idSet[id] = true
		elements = append(elements, Element{
			Href:  x.Key,
			Id:    id,
			Label: labelFromKey(x.Key),
		})
	}

	fw, err := os.OpenFile("music.inc.html", os.O_RDWR|os.O_CREATE|os.O_TRUNC, 0666)
	if err != nil {
		panic(err)
	}

	err = tmpl.Execute(fw, elements)
	if err != nil {
		panic(err)
	}

	fw.Close()
}

func main() {
	if len(os.Args) != 2 {
		fmt.Fprintf(os.Stderr, "Usage: %s <fetch|build>\n", os.Args[0])
		os.Exit(1)
	}

	subcmd := os.Args[1]
	switch subcmd {
	case "fetch":
		GoFetch()
	case "build":
		GoBuild()
	default:
		fmt.Fprintf(os.Stderr, "invalid subcmd %#v\n", subcmd)
	}
}
