#!/usr/bin/env node

// webserver script used to serve jogs.html on localhost

// it solves two problems related to web security:

// - If AUTO_LOAD_SONG_SRC is set in top of jogs.html it loads audio data
//   directly (using fetch()); this is not allowed when jogs.html is served
//   using a file:///...-link, so a http server is necessary (yes, you can load
//   it with an <audio>-tag but then you're not allowed to extract raw samples
//   from it)

// - jogs.html also uses SharedArrayBuffer; this is only supported in a "secure
//   context", which either requires https (hard to do locally) or special
//   Cross-Origin-* headers (if it wasn't for this I'd just use
//   `python -mhttp.server` on the command-line)

const PORT = 8000;

const http = require('http');
const fs = require('fs');
const path = require('path');
http.createServer((req, res) => {
	const ext_mime_map = {
		".html": "text/html",
		".mpga": "audio/mpeg",
		".mp3":  "audio/mpeg",
		".wav":  "audio/vnd.wav",
		".aiff": "audio/aiff",
		".aif":  "audio/aiff",
		".flac": "audio/flac",
		".ogg":  "audio/ogg",
		".oga":  "audio/ogg",
	};
	const local_path = path.join(".", req.url === '/' ? 'jogs.html' : req.url);
	const mime = ext_mime_map[path.extname(local_path)];
	((local_path, mime) => {
		console.log("GET " + req.url + " => " + local_path + " / mime=" + mime);
		fs.readFile(local_path, (err, data) => {
			if (err) {
				console.log(err);
				res.statusCode = 404;
				res.end('404 file not found');
				return;
			}
			res.setHeader("Content-Type", mime);
			res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
			res.setHeader("Cross-Origin-Embedder-Policy", "require-corp");
			res.end(data);
		});
	})(local_path, mime);
}).listen(PORT, () => {
	console.log("Now serving:");
	console.log("http://localhost:" + PORT);
});
