#!/usr/bin/env node

const cheerio = require('cheerio');
const path = require('path');
const fs = require('fs');

const common = require('./common');

// § 7. Til Folketinget vælges i alt 179 medlemmer, heraf 2 medlemmer på
// Færøerne og 2 medlemmer i Grønland.
const antal_mandater = 179;
const antal_nordatlantiske_mandater = 4;
const antal_danske_mandater = antal_mandater - antal_nordatlantiske_mandater;


// § 8. Landet er inddelt i tre landsdele: Hovedstaden, Sjælland-Syddanmark og
// Midtjylland-Nordjylland, jf. bilaget til loven (valgkredsfortegnelsen).

// Stk. 2. Landsdelene er inddelt i storkredse, jf. valgkredsfortegnelsen.
// Hovedstaden består af fire storkredse. Sjælland-Syddanmark og
// Midtjylland-Nordjylland består af hver tre storkredse.
const antal_storkredse = 10;

// Stk. 3. Storkredsene er inddelt i opstillingskredse, jf.
// valgkredsfortegnelsen.

// Stk. 4. En opstillingskreds består af en eller flere kommuner helt eller
// delvis. For opstillingskredse, hvori flere kommuner indgår helt eller
// delvis, udføres de fælles funktioner i den kommune, der i
// valgkredsfortegnelsen er angivet som kredskommune.


// § 9. Hver kommune eller del af en kommune i en opstillingskreds er inddelt i
// afstemningsområder. En kommune eller del af en kommune kan dog udgøre ét
// afstemningsområde. Kommunalbestyrelsen træffer beslutning om oprettelse,
// ændring eller nedlæggelse af afstemningsområder.


// § 10. Af landets 175 mandater er 135 kredsmandater og 40 tillægsmandater.
// Fordelingen af mandaterne på landsdele og på storkredse fastsættes og
// bekendtgøres af økonomi- og indenrigsministeren efter offentliggørelsen af
// folketallet pr. 1. januar 2010, 2015, 2020 osv., og fordelingen gælder
// derefter for de følgende valg.
common.assert(antal_danske_mandater == 175);
const antal_kredsmandater = 135;
const antal_tillægsmandater = 40;
common.assert((antal_kredsmandater + antal_tillægsmandater) === antal_danske_mandater);

// Stk. 2. Fordelingen foretages på grundlag af forholdstal, der for hver
// landsdel og hver storkreds beregnes som summen af landsdelens, henholdsvis
// storkredsens: 1) folketal, 2) vælgertal ved sidste folketingsvalg og 3)
// areal i kvadratkilometer multipliceret med 20. Hvis de mandattal, der
// fremkommer ved fordelingen, ikke er hele tal og derfor tilsammen ikke giver
// det fornødne antal mandater, når brøkerne bortkastes, forhøjes de største
// brøker, indtil antallet er nået (den største brøks metode). Er to eller
// flere brøker lige store, foretages lodtrækning.

// Stk. 3. Efter beregningsmetoden i stk. 2 fordeles først de 175 mandater på
// de 3 landsdele. Derefter fordeles på tilsvarende måde de 135 kredsmandater
// på landsdelene. Endelig fordeles kredsmandaterne på de enkelte storkredse
// inden for landsdelen.

// Stk. 4. Hvis der ved beregningen efter stk. 3 ikke tilfalder Bornholms
// Storkreds mindst 2 kredsmandater, foretages en fornyet fordeling af
// kredsmandaterne, hvor der forlods tillægges Bornholms Storkreds 2
// kredsmandater. De resterende 133 kredsmandater fordeles endeligt på de
// øvrige storkredse som angivet i stk. 3.

// Stk. 5. Antallet af tillægsmandater, der skal tilfalde hver landsdel,
// beregnes som forskellen mellem det samlede mandattal i landsdelen og
// antallet af kredsmandater i landsdelen.

// https://valg.im.dk/valg/folketingsvalg/kreds-og-tillaegsmandaters-stedlige-fordeling-ved-folketingsvalg

// I medfør af § 10, stk. 1, i folketingsvalgloven er der fastsat følgende
// fordeling af kreds- og tillægsmandater på landsdele og storkredse uden for
// Færøerne og Grønland:

//   Hovedstaden             tillægges 51 mandater, heraf 40 kredsmandater og 11 tillægsmandater
//   Sjælland-Syddanmark     tillægges 64 mandater, heraf 49 kredsmandater og 15 tillægsmandater
//   Midtjylland-Nordjylland tillægges 60 mandater, heraf 46 kredsmandater og 14 tillægsmandater


// https://www.retsinformation.dk/eli/lta/2020/203

// § 1. Bekendtgørelsen fastsætter fordelingen af kreds- og tillægsmandater på
// landsdele og storkredse uden for Færøerne og Grønland, jf. stk. 2-4.

// Stk. 2. Der tillægges Hovedstaden 51 mandater, heraf 40 kredsmandater og 11
// tillægsmandater. Kredsmandaterne fordeles således på landsdelens enkelte
// storkredse:

// 1) Københavns Storkreds tillægges 17 kredsmandater.

// 2) Københavns Omegns Storkreds tillægges 11 kredsmandater.

// 3) Nordsjællands Storkreds tillægges 10 kredsmandater.

// 4) Bornholms Storkreds tillægges 2 kredsmandater.

// Stk. 3. Der tillægges Sjælland-Syddanmark 64 mandater, heraf 49
// kredsmandater og 15 tillægsmandater. Kredsmandaterne fordeles således på
// landsdelens enkelte storkredse:

// 1) Sjællands Storkreds tillægges 20 kredsmandater.

// 2) Fyns Storkreds tillægges 12 kredsmandater.

// 3) Sydjyllands Storkreds tillægges 17 kredsmandater.

// Stk. 4. Der tillægges Midtjylland-Nordjylland 60 mandater, heraf 46
// kredsmandater og 14 tillægsmandater. Kredsmandaterne fordeles således på
// landsdelens enkelte storkredse:

// 1) Østjyllands Storkreds tillægges 18 kredsmandater.

// 2) Vestjyllands Storkreds tillægges 13 kredsmandater.

// 3) Nordjyllands Storkreds tillægges 15 kredsmandater.



const struktur = [
	{
		type:                  "landsdel",
		navn:                  "Hovedstaden",
		dst_id:                7,
		antal_kredsmandater:   40,
		antal_tillægsmandater: 11,
		dybere: [
			// 1) Københavns Storkreds tillægges 17 kredsmandater.
			// 2) Københavns Omegns Storkreds tillægges 11 kredsmandater.
			// 3) Nordsjællands Storkreds tillægges 10 kredsmandater.
			// 4) Bornholms Storkreds tillægges 2 kredsmandater.
			{
				type:                 "storkreds",
				navn:                 "Københavns Storkreds",
				dst_id:               10,
				kmd_id:               "KØBENHAVN",
				antal_kredsmandater:  17,
			},
			{
				type:                 "storkreds",
				navn:                 "Københavns Omegns Storkreds",
				dst_id:               11,
				kmd_id:               "KØBENHAVNS_OMEGN",
				antal_kredsmandater:  11,
			},
			{
				type:                 "storkreds",
				navn:                 "Nordsjællands Storkreds",
				dst_id:               12,
				kmd_id:               "NORDSJÆLLAND",
				antal_kredsmandater:  10,
			},
			{
				type:                 "storkreds",
				navn:                 "Bornholms Storkreds",
				dst_id:               13,
				kmd_id:               "BORNHOLM",
				antal_kredsmandater:  2,
			},
		],
	},
	{
		type:                  "landsdel",
		navn:                  "Sjælland-Syddanmark",
		dst_id:                8,
		antal_kredsmandater:   49,
		antal_tillægsmandater: 15,
		dybere: [
			// 1) Sjællands Storkreds tillægges 20 kredsmandater.
			// 2) Fyns Storkreds tillægges 12 kredsmandater.
			// 3) Sydjyllands Storkreds tillægges 17 kredsmandater.
			{
				type:                 "storkreds",
				navn:                 "Sjællands Storkreds",
				dst_id:               14,
				kmd_id:               "SJÆLLAND",
				antal_kredsmandater:  20,
			},
			{
				type:                 "storkreds",
				navn:                 "Fyns Storkreds",
				dst_id:               15,
				kmd_id:               "FYN",
				antal_kredsmandater:  12,
			},
			{
				type:                 "storkreds",
				navn:                 "Sydjyllands Storkreds",
				dst_id:               16,
				kmd_id:               "SYDJYLLAND",
				antal_kredsmandater:  17,
			},
		],

	},
	{
		type:    "landsdel",
		navn:    "Midtjylland-Nordjylland",
		dst_id:  9,
		antal_kredsmandater:   46,
		antal_tillægsmandater: 14,
		dybere: [
			// 1) Østjyllands Storkreds tillægges 18 kredsmandater.
			// 2) Vestjyllands Storkreds tillægges 13 kredsmandater.
			// 3) Nordjyllands Storkreds tillægges 15 kredsmandater.
			{
				type:                 "storkreds",
				navn:                 "Østjyllands Storkreds",
				dst_id:               17,
				kmd_id:               "ØSTJYLLAND",
				antal_kredsmandater:  18,
			},
			{
				type:                 "storkreds",
				navn:                 "Vestjyllands Storkreds",
				dst_id:               18,
				kmd_id:               "VESTJYLLAND",
				antal_kredsmandater:  13,
			},
			{
				type:                 "storkreds",
				navn:                 "Nordjyllands Storkreds",
				dst_id:               19,
				kmd_id:               "NORDJYLLAND",
				antal_kredsmandater:  15,
			},
		],
	},
];

const udregn_sum_af_kredsmandater = (xs) => {
	let sum = 0;
	for (const x of xs) sum += x.antal_kredsmandater;
	return sum;
};

const udregn_sum_af_tillægsmandater = (xs) => {
	let sum = 0;
	for (const x of xs) sum += x.antal_tillægsmandater;
	return sum;
};

// check summene
{
	common.assert(udregn_sum_af_tillægsmandater(struktur) === antal_tillægsmandater);
	common.assert(udregn_sum_af_kredsmandater(struktur) === antal_kredsmandater);
	let n_storkredse = 0;
	for (const landsdel of struktur) {
		common.assert(landsdel.type === "landsdel");
		//console.log([ landsdel.antal_kredsmandater, udregn_sum_af_kredsmandater(landsdel.dybere) ]);
		common.assert(landsdel.antal_kredsmandater === udregn_sum_af_kredsmandater(landsdel.dybere));
		n_storkredse += landsdel.dybere.length;
	}
	common.assert(n_storkredse === antal_storkredse);
}


const for_hver_storkreds = (callback) => {
	for (const landsdel of struktur) {
		for (const storkreds of landsdel.dybere) {
			callback(storkreds);
		}
	}
};

const load_dst_doc = (id) => {
	const ROOT = "_dst_data_for_fv22"; // XXX TODO
	return cheerio.load(fs.readFileSync(path.join(ROOT, "fintal_"+id+".xml")), {xmlMode:true});
};

const intval = (v) => {
	v = parseInt(v,10);
	common.assert(!isNaN(v));
	common.assert(v >= 0);
	return v;
};

const load_storkreds_stemmer = (storkreds) => {
	let $ = load_dst_doc(storkreds.dst_id);
	common.assert($("Sted").attr("Type") === "StorKreds");

	let stemmer = $("Stemmer");
	let r = {};
	stemmer.find("Parti").each((i,em) => {
		em=$(em);
		let bogstav = em.attr("Bogstav");
		if (bogstav === undefined) {
			common.assert(em.attr("Navn") === "Uden for partier");
			return;
		}
		r[bogstav] = intval(em.attr("StemmerAntal"));
	});

	let personer = $("Personer");
	personer.find("Parti").each((i,em) => {
		em=$(em);
		if (em.attr("Bogstav") !== undefined) return;
		let navn = em.attr("navn");
		common.assert(navn !== undefined);
		let antal_personlige_stemmer = intval(em.attr("PersonligeStemmer"));
		r[navn] = antal_personlige_stemmer;
	});

	return r;
};

const gcd = (a,b) => {
	while (b !== 0) {
		let t = b;
		b = a%b;
		a = t;
	}
	return a;
};
common.assert(gcd(15,5)===5);
common.assert(gcd(5,15)===5);
common.assert(gcd(33,99)===33);
common.assert(gcd(66,99)===33);
common.assert(gcd(99,66)===33);
common.assert(gcd(99,33)===33);

const lcm = (a,b) => (a*b) / gcd(a,b);
common.assert(lcm(3,5)===15);
common.assert(lcm(1,5)===5);
common.assert(lcm(6,7)===42);
common.assert(lcm(7,6)===42);


const kvotient_compar = (a,b) => {
	let m = lcm(a[1], b[1]);
	return a[0]*(m/a[1]) - b[0]*(m/b[1]);
};

// § 76. For hvert parti sammentælles de stemmer, der er tilfaldet partiet i
// samtlige opstillingskredse i storkredsen. Ligeledes sammentælles de stemmer,
// der er tilfaldet hver kandidat uden for partierne.

// Stk. 2. Hvert stemmetal, der fremkommer ved sammentællingen, jf. stk. 1,
// divideres med 1-2-3 osv., indtil der er foretaget et så stort antal
// divisioner som det antal mandater, der højst kan ventes at tilfalde partiet
// eller kandidaten uden for partierne. Det parti eller den kandidat uden for
// partierne, der har den største af de fremkomne kvotienter, får det første
// mandat i storkredsen. Den næststørste kvotient giver ret til det andet
// mandat og så fremdeles, indtil alle storkredsens kredsmandater er fordelt
// mellem partierne og kandidaterne uden for partierne. Er to eller flere
// kvotienter lige store, foretages lodtrækning.

for_hver_storkreds((storkreds) => {
	const { navn, antal_kredsmandater } = storkreds;
	const stemmer = load_storkreds_stemmer(storkreds);
	const keys = Object.keys(stemmer);
	let divisorer = {};
	let kredsmandater = {};
	for (const k of keys) {
		divisorer[k] = 1;
		kredsmandater[k] = 0;
	};
	for (let mandat_indeks = 0; mandat_indeks < storkreds.antal_kredsmandater; mandat_indeks++) {
		let bedste_kvotient = undefined;
		let bedste_k = undefined;
		for (const k of keys) {
			let tæller = stemmer[k];
			let nævner = divisorer[k];
			let kvotient = [tæller,nævner];
			if (bedste_kvotient === undefined || kvotient_compar(kvotient, bedste_kvotient) >= 0) {
				if (bedste_kvotient !== undefined && kvotient_compar(kvotient, bedste_kvotient) === 0) throw new Error("LODTRÆKNING!?");
				bedste_kvotient = kvotient;
				bedste_k = k;
			}
		}
		common.assert(bedste_k !== undefined);
		kredsmandater[bedste_k]++;
		divisorer[bedste_k]++;
	}

	{
		let sum = 0;
		for (let k in kredsmandater) sum += kredsmandater[k];
		common.assert(sum === antal_kredsmandater);
	}

	//console.log([navn, kredsmandater]);
});

// § 77. Tillægsmandaterne fordeles blandt partier, der enten

// 1) har opnået mindst ét kredsmandat, eller

// 2) inden for hver af to af de tre landsdele, der er nævnt i § 8, stk. 1, har
// opnået mindst lige så mange stemmer som det gennemsnitlige antal gyldige
// stemmer, der i landsdelen er afgivet pr. kredsmandat, eller

// 3) i hele landet har opnået mindst 2 pct. af de afgivne gyldige stemmer.

// Stk. 2. Det opgøres, hvor mange stemmer der i hele landet er tilfaldet hvert
// af de partier, der er berettiget til tillægsmandater efter stk. 1. Det
// samlede stemmetal for disse partier divideres med tallet 175 med fradrag af
// det antal kredsmandater, der måtte være tilfaldet kandidater uden for
// partierne. Med det tal, der herved fremkommer, divideres hvert af partiernes
// stemmetal. De herved fremkomne kvotienter angiver, hvor mange mandater hvert
// parti i forhold til stemmetal er berettiget til. Hvis disse kvotienter ikke
// er hele tal og derfor tilsammen ikke giver det hele antal mandater, når
// brøkerne bortkastes, forhøjes de største brøker, indtil antallet er nået
// (den største brøks metode). Er to eller flere brøker lige store, foretages
// lodtrækning.

// Stk. 3. Hvis ingen partier har opnået flere kredsmandater end det samlede
// mandattal, som partiet i forhold til sit stemmetal er berettiget til, jf.
// stk. 2, er fordelingen i stk. 2 endelig. Det antal tillægsmandater, der
// tilkommer de enkelte partier, beregnes herefter som forskellen mellem
// partiets samlede mandattal og dets kredsmandater.

// Stk. 4. Hvis et parti har opnået flere kredsmandater end det samlede
// mandattal, som partiet i forhold til sit stemmetal er berettiget til, jf.
// stk. 2, foretages en ny beregning. Ved denne beregning ses bort fra partier,
// som har opnået et antal kredsmandater lig med eller større end det samlede
// mandattal, som de er berettiget til i forhold til deres stemmetal. For de
// partier, der herefter kommer i betragtning, sker fordelingen af mandaterne
// efter tilsvarende regler som i stk. 2, og antallet af tillægsmandater, der
// tilfalder de enkelte partier, beregnes som anført i stk. 3.

// Stk. 5. Hvis et parti efter den fornyede beregning har opnået flere mandater
// end det mandattal, som partiet i forhold til sit stemmetal er berettiget
// til, jf. stk. 2, får partiet tildelt det mandattal, som er beregnet i
// henhold til stk. 2. Der foretages en ny fordeling af de resterende mandater
// på de øvrige partier efter tilsvarende regler som i stk. 2 og 3.


// § 78. For hvert af de partier, der ifølge § 77 skal have tillægsmandater,
// opgøres, hvor mange stemmer partiet har fået i hver af de 3 landsdele.

// Stk. 2. Hvert af disse stemmetal divideres med tallene 1-3-5-7 osv. Derefter
// udelades så mange af de største kvotienter, som svarer til det antal
// kredsmandater, som partiet har fået i landsdelen ifølge § 76.

// Stk. 3. Den landsdel og det parti, der herefter har den største kvotient,
// får det første tillægsmandat. Den landsdel og det parti, der har den
// næststørste kvotient, får det næste tillægsmandat og så fremdeles. Når en
// landsdel eller et parti har fået det antal tillægsmandater, som den eller
// det skal have, jf. §§ 10 og 77, kommer landsdelen eller partiet ikke videre
// i betragtning. Fordelingen fortsættes for de andre landsdele og for de
// øvrige partier, indtil samtlige tillægsmandater er fordelt. Hvis et parti,
// der ikke har fået stemmer i alle 3 landsdele, ved denne fordeling ikke kan
// få tildelt de tillægsmandater, som partiet er berettiget til, skal disse
// forlods tildeles partiet i de landsdele, hvor der er afgivet stemmer for
// det.


// § 79. Inden for den eller de landsdele, hvor et parti har fået
// tillægsmandater ifølge § 78, divideres partiets stemmetal i de enkelte
// storkredse med tallene 1-4-7-10 osv. I hver storkreds udelades derefter så
// mange af de største kvotienter, som svarer til det antal kredsmandater, som
// partiet har fået i storkredsen.

// Stk. 2. Den storkreds, der herefter har den største kvotient, får det første
// tillægsmandat. Det næste tillægsmandat tilfalder den storkreds, der har den
// næststørste kvotient, og så fremdeles, indtil det antal tillægsmandater, som
// partiet har fået i landsdelen, er fordelt.

// Stk. 3. Er ved fordelingen af tillægsmandaterne på landsdele eller på
// storkredse to eller flere kvotienter lige store, foretages lodtrækning.






