#!/usr/bin/env node

const cheerio = require('cheerio');
const path = require('path');
const url = require('url');
const fs = require('fs');
const fsp = require('fs/promises');

const common = require('./common');

const target = common.select([
	[   "fv22",   "Valg1968094",   "Folketingsvalg 1. november 2022"  ],
	[   "fv19",   "Valg1684447",   "Folketingsvalg 5. juni 2019"      ],
]);

common.enter_target_dir(target, "dst");

const ROOT_URL = "https://www.dst.dk/" + path.join("valg", target[1], "xml") + "/";
console.log("Root URL: " + ROOT_URL);

const Bottleneck = require("bottleneck");
const rate_limiter = new Bottleneck({ maxConcurrent: 4 });
const fetch = require('cross-fetch');
const rate_limited_fetch = rate_limiter.wrap(fetch);

const logp = (msg) => new Promise((resolve) => { console.log(msg); resolve(); });
const load_xml = (xml) => cheerio.load(xml, {xmlMode:true});
const stor = (p, doc) => logp("Gemmer " + p + "...").then(() => fsp.writeFile(p, doc.xml()));
const get_xml = (url) => rate_limited_fetch(url).then(r=>r.text()).then(xml => load_xml(xml));
const relative_path_url = (relative_path) => (ROOT_URL + relative_path);
const xml_archive = (p) => fsp.stat(p).then(
	() => fsp.readFile(p).then((blob) => load_xml(blob))
).catch(
	() => get_xml(relative_path_url(p)).then(doc => stor(p, doc).then(() => doc))
);

const url_xml_archive = (url) => xml_archive(strip_root_url(url));

const strip_root_url = (url) => {
	common.assert(url.startsWith(ROOT_URL));
	return url.slice(ROOT_URL.length);
};

const ordleg_archive = ($, outer, inner) => {
	outer = $(outer);
	common.assert(outer.length === 1);
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
