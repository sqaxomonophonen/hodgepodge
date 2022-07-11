#!/usr/bin/env python3
import os,json
o = json.loads(open("music.json").read())
keys = [x["Key"] for x in o["Contents"]]

with open("musicdata.js", "w") as f:
	f.write("MUSICDATA=")
	f.write(json.dumps(keys))
	f.write(";\n")
