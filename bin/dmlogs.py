#!/usr/bin/env python3
import sys,urllib.parse,json

def parse_clf_line(line, line_number = -1):
	remaining = line
	schema = [("s","host"), ("s","ident"), ("s","authuser"), ("[","date"), ("s","request"), ("i","status"), ("i","bytes"), ("s","referrer"), ("s","user_agent")]
	o = {}
	for typ,key in schema:
		remaining = remaining.lstrip()
		value = None
		if len(remaining) == 0:
			sys.stderr.write("ERROR: missing \"%s\" on line %d (%s)\n" % (key, line_number, line))
			return None
		if typ == "s":
			if remaining[0] == "\"":
				remaining = remaining[1:]
				value = ""
				while True:
					escape_pos = remaining.find("\\")
					end_pos = remaining.find("\"")
					if end_pos == -1:
						sys.stderr.write("ERROR: string not terminated for key \"%s\" on line %d (%s)\n" % (key, line_number, line))
						return None
					elif escape_pos >= 0 and escape_pos < end_pos:
						value += remaining[:escape_pos] + remaining[escape_pos+1]
						remaining = remaining[escape_pos+2:]
					else:
						value += remaining[:end_pos]
						remaining = remaining[end_pos+1:]
						break
			else:
				end_pos = remaining.find(" ")
				if end_pos == -1: end_pos = len(remaining)
				value = remaining[:end_pos]
				remaining = remaining[end_pos+1:]
		elif typ == "[":
			end_pos = remaining.find("]")
			if end_pos == -1:
				sys.stderr.write("ERROR: date not terminated for key \"%s\" on line %d (%s)\n" % (key, line_number, line))
				return None
			value = remaining[1:end_pos] # TODO parse data if desired?
			remaining = remaining[end_pos+1:]
		elif typ == "i":
			end_pos = remaining.find(" ")
			if end_pos == -1: end_pos = len(remaining)
			s = remaining[:end_pos]
			value = None if s=="-" else int(s)
			remaining = remaining[end_pos+1:]
		else:
			raise AssertionError("bad typ %s" % typ)
		o[key] = value

	return o


def print_table(tbl):
	if len(tbl) == 0: return
	widths = [0] * len(tbl[0])
	for row in tbl:
		for i in range(len(row)):
			widths[i] = max(widths[i], len(str(row[i])))
	pad = 3
	for row in tbl:
		s = ""
		for i in range(len(row)):
			s += str(row[i]).ljust(widths[i]+pad)
		print(s.rstrip())



# "best effort", obviously
def classify_user_agent(user_agent):
	if user_agent is None: return None
	lcua = user_agent.lower()
	bots = [
		"bot.html", "/robots",
		"googlebot", "googleother", "google-safety",
		"bingbot", "ccbot", "akkoma", "claudebot", "iabot", "petalbot",
		"amazonbot", "applebot", "msnbot", "twitterbot",
		"yacybot", "youbot", "semrushbot", "yandexbot",
		"coccocbot-web", "dataforseobot", "mj12bot", "imagesiftbot",
		"dotbot", "mojeekbot", "seznambot",
		"baiduspider", "bytespider", "yisouspider", "sogou",
		"wordpress", "newsblur", "rss-feed-indexer", "feedly",
		"feedbin", "akregator", "montastic-monitor", "masscan",
		"go-http-client", "apache-httpclient", "python-requests", "php/", "curl",
		"palo alto networks", "facebookexternalhit",
		"marginalia.nu", "ia_archiver", "uptimerobot",
	]
	for bot in bots:
		if bot in lcua: return "bot"
	#print(user_agent)
	return "human"

def fmt_byte_count(n):
	suffixes = ["B", "kB", "MB", "GB", "TB"]
	div = 1
	i = 0
	M = 1024
	while n >= div*M and i < len(suffixes)-1:
		div *= M
		i += 1
	return "%.2f %s" % (n/div, suffixes[i])


def print_top(what, kn, MAX = 10):
	if not kn or len(kn) == 0: return
	if not kn or len(kn) == 0: return
	print()
	tbl = [(x[1],x[0]) for x in sorted(kn.items(), key = lambda x: -x[1])]
	if len(tbl) <= MAX:
		print("%s:" % what)
	else:
		print("%s (showing top %d):" % (what, MAX))
		tbl = tbl[:MAX]
	print_table(tbl)


def cmd_stat():
	"""Print log summary for log lines via stdin"""
	line_number = 1
	n_5xx = 0
	n_4xx = 0
	n_oks = 0
	n_redirects = 0
	n_requests = 0
	first_5xx = None
	last_5xx = None
	first_request = None
	last_request = None
	n_bytes_served = 0
	n_bot_requests = 0
	n_human_requests = 0
	server_errors = {}
	referrals = {}
	for line in sys.stdin.readlines():
		o = parse_clf_line(line, line_number)
		if o is None: continue
		c = o["status"]
		cls = classify_user_agent(o["user_agent"])
		if cls == "bot":
			n_bot_requests += 1
		elif cls == "human":
			n_human_requests += 1
		n_requests += 1
		if o["bytes"] is not None: n_bytes_served += o["bytes"]

		ref = o["referrer"]
		if ref and ref != "-" and ref != "":
			host = urllib.parse.urlparse(ref).netloc.lower()
			if len(host) > 0:
				if host not in referrals:
					referrals[host] = 0
				referrals[host] += 1

		if c is not None:
			date = o["date"]
			if first_request is None: first_request = date
			last_request = date
			ms = c // 100
			if ms == 2:
				n_oks += 1
			elif ms == 3:
				n_redirects += 1
			elif ms == 4:
				n_4xx += 1
			elif ms == 5:
				n_5xx += 1
				if first_5xx is None: first_5xx = date
				last_5xx = date
				rq = o["request"]
				if rq not in server_errors: server_errors[rq] = 0
				server_errors[rq] += 1

		line_number += 1

	tbl = [
		("Requests in total", n_requests),
		("Bot requests (estimate)", n_bot_requests),
		("Human requests (estimate)", n_human_requests),
		("(2xx) OKs", n_oks),
		("(3xx) Redirects", n_redirects),
		("(4xx) Client errors", n_4xx),
		("(5xx) Server errors", n_5xx),
	]
	if n_5xx > 0:
		tbl.extend([
			("First server error", first_5xx),
			("Last server error", last_5xx),
		])
	tbl.extend([
		("Bytes served", str(n_bytes_served) + " (" + fmt_byte_count(n_bytes_served) + ")"),
		("First request", first_request),
		("Last request", last_request),
	])

	print_table(tbl)
	print_top("Referrals", referrals, 15)
	print_top("Server errors", server_errors, 10)


def cmd_5xx():
	"''Grep'' stdin for lines with 5xx as response code"
	line_number = 1
	for line in sys.stdin.readlines():
		o = parse_clf_line(line, line_number)
		if o["status"] is None or o["status"]//100 != 5: continue
		sys.stdout.write(line)
		line_number += 1

def cmd_json():
	"Convert log lines from stdin to JSON"
	xs = []
	line_number = 1
	for line in sys.stdin.readlines():
		x = parse_clf_line(line, line_number)
		if x is None: continue
		xs.append(x)
		line_number += 1
	print(json.dumps(xs))


cmd_prefix = "cmd_"
def usage():
	prg = sys.argv[0]
	sys.stderr.write("Usage: %s <cmd>\n" % prg)
	for name, fn in [v for v in globals().items() if callable(v[1]) and v[0].startswith(cmd_prefix)]:
		sys.stderr.write("  %s %s %s\n" % (prg, name.removeprefix(cmd_prefix).ljust(10), fn.__doc__))
	sys.exit(1)
if len(sys.argv) != 2: usage()
fname = cmd_prefix + sys.argv[1]
if fname not in vars(): usage()
vars()[fname]()
