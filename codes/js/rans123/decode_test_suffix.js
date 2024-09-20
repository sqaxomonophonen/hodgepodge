let cum_freq_to_symidx = []
let cursor = 0;
let sym_start = [];
let sym_freq = [];
for (let i0 = 0; i0 < S.length; i0++) {
	const s = S[i0];
	if (s === null) continue;
	const freq = s[1];
	sym_start[i0] = cursor;
	sym_freq[i0] = freq;
	for (let i1 = 0; i1 < freq; i1++) {
		cum_freq_to_symidx[cursor++] = i0;
	}
}
let src = "";
for (let i = 0; i < 4795; i++) {
	let cum_freq = X();
	let symidx = cum_freq_to_symidx[cum_freq];
	if (symidx === undefined) throw new Error("XXX");
	let start = sym_start[symidx];
	let freq = sym_freq[symidx];
	//console.log([S[symidx][0],start,freq]);
	src += S[symidx][0];
	X(start, freq);
}
console.log(src);
eval(src);
