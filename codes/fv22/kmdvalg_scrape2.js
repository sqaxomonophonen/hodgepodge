#!/usr/bin/env node

const cheerio = require('cheerio');
const path = require('path');
const url = require('url');
const fs = require('fs');
const fsp = require('fs/promises');

const common = require('./common');

const target = common.select([
	[   "fv22",   "https://kmdvalg.dk/Main/Home/FV",                "Folketingsvalg 1. november 2022"  ],
	[   "fv19",   "https://www.kmdvalg.dk/fv/2019/KMDValgFV.html",  "Folketingsvalg 5. juni 2019"      ],
]);

common.enter_target_dir(target, "kmdvalg");

class KeyPath {
	constructor() {
		this.path = [];
	}

	enter(fragment) {
		let o = new KeyPath();
		o.path = [...this.path, fragment];
		return o;
	}

	get(index) {
		common.assert(0 <= index && index < this.path.length);
		return this.path[index];
	}

	fspath() {
		return this.path.join("__") + ".json";
	}

	archive(data) {
		let p = this.fspath();
		fsp.writeFile(p, JSON.stringify(data,null,2)).then(() => {
			console.log("Gemte " + p + " ...");
		});
	}
};

const Bottleneck = require("bottleneck");
const rate_limiter = new Bottleneck({ maxConcurrent: 2 });
const fetch = require('cross-fetch');
const rate_limited_fetch = rate_limiter.wrap(fetch);

const get_html = url => rate_limited_fetch(url).then(x=>x.text()).then(x => new Promise(r => r(cheerio.load(x))));

const url_resolv = (a,b) => (new url.URL(b,a)).href;

const label_defs = [
	[  'Antal stemmeberettigede:',   "antal_stemmeberettigede",   "int"        ],
	[  'Ajourført:',                 "ajourført",                 "date"       ],
	[  'Blanke stemmer:',            "blanke_stemmer",            "int+diff"   ],
	[  'Ugyldige stemmer:',          "ugyldige_stemmer",          "int+diff"   ],
	[  'I alt gyldige stemmer:',     "gyldige_stemmer",           "int+diff"   ],
	[  'I alt afgivne stemmer:',     "afgivne_stemmer",           "int+diff"   ],
	[  'Sted:',                      "sted",                      "text"       ],
	[  'Partistemmer:',              "partistemmer",              "int"        ],
	[  'I alt stemmer:',             "stemmer",                   "int"        ],

	// ignore
	['Øvrige stemmer fra forrige valg:'],
	['Status på optælling:'],
	['Optalte afstemningssteder:'],
];

const parti_defs = [
	["A", "#db0812", "Socialdemokratiet",                  ],
	["B", "#df0078", "Radikale Venstre",                   ],
	["C", "#00674f", "Det Konservative Folkeparti",        ],
	["D", "#00708d", "Nye Borgerlige",                     ],
	["F", "#b70e0c", "Socialistisk Folkeparti",            ],
	["I", "#003f4d", "Liberal Alliance",                   ],
	["K", "#eb9900", "Kristendemokraterne",                ],
	["M", "#6e387b", "Moderaterne",                        ],
	["O", "#fcd500", "Dansk Folkeparti",                   ],
	["Q", "#a0cc8b", "Frie Grønne",                        ],
	["V", "#0081c3", "Venstre",                            ],
	["Æ", "#4fc2e9", "Danmarksdemokraterne",               ],
	["Ø", "#89150d", "Enhedslisten",                       ],
	["Å", "#65b32e", "Alternativet",                       ],
];

const label_map = common.map_from_array_of_tuples(label_defs, 0);
const parti_map = common.map_from_array_of_tuples(parti_defs, 0);

const get_label_tuple = (label) => {
	let tuple = label_map[label];
	if (tuple === undefined) throw new Error("no tuple for label: " + label);
	return tuple;
};

const parse_int = (v) => parseInt(v.replace(".",""), 10);
const parse_float = (v) => parseFloat(v.replace(".","").replace(",","."));

const value_parsers = {
	"int":      (tds) => ({"value": parse_int(tds.eq(1).text())}),
	"text":     (tds) => ({"text": tds.eq(1).text().trim().replace(/  +/gm, " / ")}),
	"date":     (tds) => ({"date": tds.eq(1).text()}),
	"int+diff": (tds) => ({"value": parse_int(tds.eq(1).text()), "diff": parse_int(tds.eq(2).text())}),
};

const href2id = (href) => path.basename(href).split(".")[0];

const parse_info_table = ($, keypath) => {
	let ob;

	let tables = $(".kmd-table-info");
	common.assert(tables.length >= 1, keypath.fspath());

	let t0 = $(tables.eq(0));

	ob = {};
	t0.find("tr").each(function(i,em) {
		em = $(em);
		let tds = $(em.find("td"));
		let label = tds.eq(0).text();
		let tuple = get_label_tuple(label);
		let key = tuple[1];
		if (key === undefined) return;
		let parser = value_parsers[tuple[2]];
		if (parser === undefined) throw new Error("no value parser: " + tuple[2]);
		ob[key] = parser(tds);
	});

	return ob;
}

const parse_parti_table = ($) => {
	let xs = $(".kmd-parti-list");
	common.assert(xs.length === 1);
	let table = [];
	$(xs).find(".table-like-row").each(function(i0,em) {
		if (i0 < 1) return;
		em = $(em);

		let row = {};
		em.find(".table-like-cell").each(function(i1,em) {
			em = $(em);
			if (i1 === 0) {
				let bogstav = em.find(".parti-letter").text();
				row["bogstav"] = bogstav;
				if (!parti_map[bogstav]) {
					row["navn"] = em.text().replace(bogstav, ""); // XXX too ugly? :)
				}
				let a = em.find("a");
				if (a.length > 0) {
					row["href"] = a.attr("href");
					row["id"] = row["href"].toLocaleUpperCase().split(".")[0];
				}

			} else if (i1 === 1) {
				row["stemmetal"] = parse_int(em.text());
			} else if (i1 === 2) {
				row["stemmetal_diff"] = parse_int(em.text());
			} else if (i1 === 3) {
				row["percent"] = parseFloat(em.find(".kmd-progress-percent").attr("data-count-to"));
			}
		});
		table.push(row);
	});
	return table;
};

function crawl_parti_page(keypath, root) {
	get_html(root).then($ => {
		let info = parse_info_table($, keypath);

		let vs = $(".kmd-personal-votes-list");
		common.assert(vs.length === 1);

		let personlige_stemmer = [];
		$(vs).find(".table-like-row").each(function(i0,em) {
			if (i0 < 1) return;
			em = $(em);

			let row = {};
			em.find(".table-like-cell").each(function(i1,em) {
				em = $(em);
				if (i1 === 0) {
					row["navn"] = em.text();
				} else if (i1 === 1) {
					row["stemmer"] = parse_int(em.text());
				}
			});
			personlige_stemmer.push(row);
		});

		let data = { info, personlige_stemmer }
		keypath.archive(data);
	});
}

function crawl_parti_table(parent_keypath, parti, parent_root) {
	for (const row of parti) {
		if (!row.href || !row.id) continue;
		crawl_parti_page(parent_keypath.enter(row.id), url_resolv(parent_root, row.href));
	}
}

function crawl_rec(keypath, navn, root) {
	get_html(root).then($ => {
		let info = parse_info_table($, keypath);

		let parti = parse_parti_table($);
		crawl_parti_table(keypath, parti, root);

		let dybere = null;
		let g = $("#vote-areas");
		if (g.length > 0) {
			dybere = [];
			g.find("a").each(function(i,em) {
				em = $(em);
				let href = em.attr("href");
				let id01 = href2id(href);
				let dybere_navn = em.text();
				dybere.push({id:id01, navn:dybere_navn});
				crawl_rec(keypath.enter(id01), dybere_navn, url_resolv(root, href));
			})
		}

		let data = { navn, info, dybere, parti };
		keypath.archive(data);
	});
}

const fragment_sanitize = (s) => s.toLocaleUpperCase().replace(" ", "_");

function crawl_lvl0(root) {
	get_html(root).then($ => {
		let d = $(".kmd-list-items");

		d.find(".list-group").each(function(i, em) {
			em = $(em);
			let keypath = new KeyPath();
			let valgkreds = em.find("div").text();
			keypath = keypath.enter(fragment_sanitize(valgkreds));
			em.find("a").each(function(i, em) {
				em = $(em);
				let href = em.attr("href");
				let id0 = href2id(href);
				crawl_rec(keypath.enter(id0), em.text(), url_resolv(root, href));
			});
		});
	});
}

crawl_lvl0(target[1]);
