#!/usr/bin/env node

const cheerio = require('cheerio');
const fetch = require('cross-fetch');
const path = require('path');
const url = require('url');
const fs = require('fs');
const fsp = require('fs/promises');

const targets = [
	[   "fv22",   "Valg1968094",   "Folketingsvalg 1. november 2022"  ],
	[   "fv19",   "Valg1684447",   "Folketingsvalg 5. juni 2019"      ],
];

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

const target_map = map_from_array_of_tuples(targets, 0);


const PRG = path.basename(process.argv[1]);

const argv = process.argv.slice(2);

if (argv.length !== 1) {
	console.log("Usage: " + PRG + " <target>");
	console.log("Targets:");
	for (const tuple of targets) {
		console.log("  " + tuple[0] + "\t\t(" + tuple[2] + ")");
	}
	process.exit(1);
}

const target_key = argv[0];
const target = target_map[target_key];
if (target === undefined) {
	console.log("Invalid target: " + target_key);
	process.exit(1);
}

const data_toplvl_key = target[1];

const dir_to_enter = "_data_for_" + target_key;

try { fs.mkdirSync(dir_to_enter); } catch(e) {}
process.chdir(dir_to_enter);
console.log("Data: " + process.cwd() + " ...");

const ROOT_URL = "https://www.dst.dk/" + path.join("valg", data_toplvl_key, "xml") + "/";
console.log("Root URL: " + ROOT_URL);

const logp = (msg) => new Promise((resolve) => { console.log(msg); resolve(); });
const load_xml = (xml) => cheerio.load(xml, {xmlMode:true});
const stor = (p, doc) => logp("Gemmer " + p + "...").then(() => fsp.writeFile(p, doc.xml()));
const get_xml = (url) => fetch(url).then(r=>r.text()).then(xml => load_xml(xml));
const relative_path_url = (relative_path) => (ROOT_URL + relative_path);
const xml_archive = (p) => fsp.stat(p).then(
	() => fsp.readFile(p).then((blob) => load_xml(blob))
).catch(
	() => get_xml(relative_path_url(p)).then(doc => stor(p, doc).then(() => doc))
);

const url_xml_archive = (url) => xml_archive(strip_root_url(url));

const ASSERT = (p) => { if (!p) throw new Error("ASSERTION FAILED"); };

const strip_root_url = (url) => {
	ASSERT(url.startsWith(ROOT_URL));
	return url.slice(ROOT_URL.length);
};

const ordleg_archive = ($, outer, inner) => {
	outer = $(outer);
	ASSERT(outer.length === 1);
	outer.find(inner).each((i,inner) => {
		url_xml_archive($(inner).attr("filnavn"));
	});
};

const ordleg_archive_common0 = ($) => {
	ordleg_archive($, "Landsdele", "Landsdel");
	ordleg_archive($, "Storkredse", "Storkreds");
	ordleg_archive($, "Opstillingskredse", "Opstillingskreds");
	ordleg_archive($, "Afstemningsomraader", "Afstemningsomraade");
};

xml_archive("generel.xml");
xml_archive("valgdag.xml").then(($) => {
	ordleg_archive_common0($);
	$("Land").each((i, inner) => {
		url_xml_archive($(inner).attr("filnavn"));
	});
});
xml_archive("fintal.xml").then(($) => {
	ordleg_archive_common0($);
});
