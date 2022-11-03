#!/usr/bin/env node

const cheerio = require('cheerio');
const fetch = require('cross-fetch');
const path = require('path');
const url = require('url');

const XXXDBG = true;

const what = [
	[  "fv2022", "https://kmdvalg.dk/Main/Home/FV"                ],
	[  "fv2019", "https://www.kmdvalg.dk/fv/2019/KMDValgFV.html"  ],
];

const gethtml = url => fetch(url).then(x=>x.text()).then(x => new Promise(r => r(cheerio.load(x))));

const url_resolv = (a,b) => (new url.URL(b,a)).href;

const map_from_array_of_tuples = (array, tuple_key_index) => {
	let map = {}
	for (const tuple of array) {
		let key = tuple[tuple_key_index];
		if (key === undefined) throw new Error("no key");
		if (map[key] !== undefined) throw new Error("duplicate key: " + key);
		map[key] = tuple;
	}
	return map;
};

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

const label_map = map_from_array_of_tuples(label_defs, 0);
const parti_map = map_from_array_of_tuples(parti_defs, 0);

const get_label_tuple = (label) => {
	let tuple = label_map[label];
	if (tuple === undefined) throw new Error("no tuple for label: " + label);
	return tuple;
};

const parse_int = (v) => parseInt(v.replace(".",""), 10);
const parse_float = (v) => parseFloat(v.replace(".","").replace(",","."));

const value_parsers = {
	"int": (tds) => ({"value": parse_int(tds.eq(1).text())}),
	"text": (tds) => ({"text": tds.eq(1).text().trim().replace(/  +/gm, " / ")}),
	"date": (tds) => ({"date": tds.eq(1).text()}),
	"int+diff": (tds) => ({"value": parse_int(tds.eq(1).text()), "diff": parse_int(tds.eq(2).text())}),
};

const href2id = (href) => path.basename(href).split(".")[0];

const parse_info_table = ($, expected_header) => {
	expected_header = expected_header.toLocaleLowerCase();

	let ob;

	$(".content-block").each(function(i,em) {
		em = $(em);
		if (em.find(".content-block-header").text().toLocaleLowerCase() !== expected_header) {
			return;
		}
		em = em.find("table").eq(0);
		ob = {};
		em.find("tr").each(function(i,em) {
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
	});

	if (ob === undefined) throw new Error("no info table found?!");

	return ob;
}

const parse_parti_table = ($, lvl) => {
	let xs = $(".kmd-parti-list");
	if (xs.length !== 1) throw new Error("expected to find exactly one element");
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
				if (a.length > 0) row["ref"] = a.attr("href");

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

function crawl_parti_page(idxx, root) {
	if (XXXDBG && idxx !== "F1007A") return; // XXX
	gethtml(root).then($ => {
		let ob = parse_info_table($, "Valgkreds info")
		console.log(ob); // XXX

		let vs = $(".kmd-personal-votes-list");
		if (vs.length !== 1) throw new Error("expected to find exactly one element");

		let table = [];
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
			table.push(row);
		});

		console.log(table);
	});
}

function crawl_parti_table(parent_root, table) {
	for (const row of table) {
		if (!row.ref) continue;
		let idxx = href2id(row.ref).toLocaleUpperCase();
		crawl_parti_page(idxx, url_resolv(parent_root, row.ref));
	}
}

function crawl_lvl2(label, valgkreds, id0, id01, root) {
	if (XXXDBG) return; // XXX
	if (XXXDBG && id01 !== "F1007851036") return; // XXX
	gethtml(root).then($ => {
		let ob = parse_info_table($, "Afstemningsområde info")
		console.log(ob); // XXX

		let pt = parse_parti_table($, 2);
		crawl_parti_table(root, pt);
	});
}

function crawl_lvl1(label, valgkreds, id0, root) {
	if (XXXDBG && id0 !== "F1007") return; // XXX
	gethtml(root).then($ => {
		let ob = parse_info_table($, "Valgkreds info")
		console.log(ob);

		let pt = parse_parti_table($, 1);
		crawl_parti_table(root, pt);

		let g = $(".kmd-voting-areas-list-items");
		if (g.length !== 1) throw new Error("expected to find exactly one element");
		g.find("a").each(function(i,em) {
			em = $(em);
			let href = em.attr("href");
			let id01 = href2id(href);
			crawl_lvl2(label, valgkreds, id0, id01, url_resolv(root, href));
		})
	});
}

function crawl_lvl0(label, root) {
	gethtml(root).then($ => {
		let d = $(".kmd-list-items");

		d.find(".list-group").each(function(i, em) {
			em = $(em);
			let valgkreds = em.find("div").text();
			em.find("a").each(function(i, em) {
				em = $(em);
				let href = em.attr("href");
				let id0 = href2id(href);
				crawl_lvl1(label, valgkreds, id0, url_resolv(root, href));
			});
		});
	});
}

for (const tup of what) {
	crawl_lvl0(tup[0], tup[1]);
	break; // XXX
}
