#!/usr/bin/env node

const cheerio = require('cheerio');
const path = require('path');
const fs = require('fs');

const common = require('./common');

const xml_load = (rel_path) => cheerio.load(fs.readFileSync(rel_path), {xmlMode:true});

function dst_load(root) {
	let $ = xml_load(path.join(root, "fintal.xml"));

	const structure = [
		[  "Landsdel",            "landsdel_id",             ],
		[  "Storkreds",           "storkreds_id",            ],
		[  "Opstillingskreds",    "opstillingskreds_id",     ],
		[  "Afstemningsomraade",  "afstemningsomraade_id",   ],
	];

	let store = {};

	const mk_skey = (key,val) => (key+"="+val);

	const extract_fintal = (rel_path) => {
		let $ = xml_load(path.join(root, rel_path));

		let stemmer = $("Stemmer");
		common.assert(stemmer.length === 1);

		let fintal = {};

		const intval = (v) => {
			v = parseInt(v,10);
			common.assert(!isNaN(v));
			common.assert(v >= 0);
			return v;
		};

		const stemmer_intval = (k) => intval(stemmer.find(k).text());

		fintal.i_alt_gyldige_stemmer    = stemmer_intval("IAltGyldigeStemmer");
		fintal.blanke_stemmer           = stemmer_intval("BlankeStemmer");
		fintal.andre_ugyldige_stemmer   = stemmer_intval("AndreUgyldigeStemmer");
		fintal.i_alt_ugyldige_stemmer   = stemmer_intval("IAltUgyldigeStemmer");
		fintal.i_alt_afgivne_stemmer    = stemmer_intval("IAltAfgivneStemmer");

		// sanity check af værdierne...
		common.assert( fintal.i_alt_afgivne_stemmer  ===  (fintal.i_alt_gyldige_stemmer  + fintal.i_alt_ugyldige_stemmer));
		common.assert( fintal.i_alt_ugyldige_stemmer ===  (fintal.blanke_stemmer         + fintal.andre_ugyldige_stemmer));

		let parti = {};
		stemmer.find("Parti").each((i,em) => {
			em = $(em);
			let bogstav = em.attr("Bogstav");
			let antal_stemmer = em.attr("StemmerAntal");
			let k = bogstav || "";
			common.assert(parti[k] === undefined);
			parti[k] = antal_stemmer;
		});
		fintal.parti = parti;

		let personer = $("Personer");
		if (personer.length === 1) {
			personer.find("Parti").each((i,em_parti) => {
				em_parti = $(em_parti);
				let bogstav = em_parti.attr("Bogstav");
				let parti_stemmer_i_alt = em_parti.attr("StemmerIAlt");
				let optalt_stemmer_i_alt = 0;
				let parti_navne = {};
				em_parti.find("Person").each((i,em_person) => {
					em_person = $(em_person);
					let person_stemmer_i_alt = em_person.attr("StemmerIAlt");
					let navn = em_person.attr("Navn");

					// check for: https://twitter.com/nilleren/status/1588277428203364352 :-)
					if (parti_navne[navn]) console.log("ADVARSEL kandidat står dobbelt: " + navn);
					parti_navne[navn] = true;

					optalt_stemmer_i_alt += person_stemmer_i_alt;
				});
			});
		}
		// TODO tilføj personlige stemmer til `fintal`

		return fintal;
	};

	let tree = [];

	for (let level = 0; level < structure.length; level++) {
		let [ tagname, key, map ] = structure[level];
		let parent_key = level === 0 ? null : structure[level-1][1];

		$(tagname).each((i,em) => {
			em = $(em);
			let id = em.attr(key);
			let navn = em.text();
			let obj = {type:tagname, navn};

			obj.fintal = extract_fintal(path.basename(em.attr("filnavn")));

			if (level === 0) {
				tree.push(obj);
			} else {
				let parent_id = em.attr(parent_key);
				let po = store[mk_skey(parent_key, parent_id)];
				if (!po.dybere) po.dybere = [];
				po.dybere.push(obj);
			}
			store[mk_skey(key,id)] = obj;
		});
	}

	return tree;
}

function kmd_load(root) {
	throw new Error("TODO");//XXX
}

function valgdata2022_load(root) {
	throw new Error("TODO");//XXX
}

function save(rel_path, tree) {
	fs.writeFileSync(rel_path, JSON.stringify(tree, null, 3));
}

save("dst.fv22.json", dst_load("_dst_data_for_fv22"));
//save("dst.fv19.json", dst_load("_dst_data_for_fv19"));
