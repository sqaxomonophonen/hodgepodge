fs=require('fs');

const SPLIT="Z";

function LOAD(path) {
	return fs.readFileSync(path,'utf-8');
}

function COMP(name, orig, prefixes) {
	let w = orig;
	w = w.replaceAll("\n","");
	let next_token = (() => {
		let single_letter = "ABCDEFGHIJKLMNOPQRSTUVWXYabcdefghijklmnopqrstuvwxyz0123456789~@";
		if (single_letter.indexOf(SPLIT) !== -1) throw new Error("SPLIT in token list");
		let is_valid = (token) => w.indexOf(token) === -1;
		if (!is_valid(SPLIT)) throw new Error("source contains SPLIT char; please remove it, or change SPLIT");
		let isl = 0;
		return () => {
			for (;;) {
				if ((isl++) < single_letter.length) {
					const token = single_letter[isl-1];
					if (is_valid(token)) return token;
				} else {
					throw new Error("XXX ran out of characters!"); // try maybe "@"?
				}
			}
		};
	})();
	let token = next_token();
	let pairs = [];
	let ratio;
	let prev_ratio = 1;
	const get_ratio = _=>(w.length + 2 + pairs.join(SPLIT).length) / orig.length;

	for (let prefix of prefixes) {
		pairs.push(token+prefix);
		w=w.replaceAll(prefix,token);
		ratio = get_ratio();
		if (ratio > prev_ratio) {
			console.error("WARNING: prefix does not seem to compress: " + prefix)
			console.error("Bad ratio change:", prev_ratio, "=>", ratio);
		}
		prev_ratio = ratio;
		token = next_token();
	}
	console.error(name, "compression:", ratio, "( without dictionary:" , w.length/orig.length, ")");
	return [w, pairs.join(SPLIT)];
}

const clean_html = s => s.replace(/<!--[\s\S]*?-->/g, ''); // remove HTML comments
const clean_css  = s => s.replace(/\/\*[\s\S]*?\*\//g, ''); // remove CSS comments

let [ css, css_pairs ] = COMP("player3.css",clean_css(LOAD("player3.css")),[
	"filter:drop-shadow(",
	"position:absolute;",
	"position:relative;",
	"cursor:pointer;",
	"border-radius:",
	"font-family:",
	"background:",
	"opacity:",
	"px #000",
	"height:",
	"margin",
	"width:",
	"color:",
	"right:",
	"left:",
	"100%",
	"0 0 ",
	");}",
	"px",
]);

let [ html, html_pairs ] = COMP("player3.doc.html",clean_html(LOAD("player3.doc.html")),[
	'<rect width="6" height="16" y="2" style="fill:$C" x="',
	'<path stroke="$C" stroke-width="1" fill="none" id="c',
	'<svg viewBox="0 0 ',
	'<polygon points="',
	'<div class="',
	'<div id="',
	'height="',
	'fill:$C"',
	'style="',
	'class="',
	'width="',
	"</div>",
	"</svg>",
	"canvas",
	"span",
	"0 0 ",
	'" ',
	'><',
	'="',
	'/>',
	'">',
]);

let [ aw, aw_pairs ] = COMP("worklet3.min.js",LOAD("worklet3.min.js"),[
	'Processor',
	//'.length', // this is too common; handle via outer compressor?
	'this.',
	'port.'
]);

let [ player, player_pairs ] = COMP("player3.min.js",LOAD("player3.min.js"),[
	".create",
	".connect",
	".style.",
	".on",
	"new ",
	//'.length', // this is too common; handle via outer compressor?
]);

function sqjson(o) {
	return "'"+JSON.stringify(o).slice(1,-1).replaceAll('\\"','"')+"'";
}


// XXX I should probably resist the temptation to save a few chars by inlining
// A0/A1/A2, but it has a higher risk of F being overwritten...
console.log("F=(s,p)=>{for(p=p.split('"+SPLIT+"');p.length;){let x=p.pop();s=s.replaceAll(x[0],x.slice(1));}return s}");
console.log("A0=F(`<style>"+css+"</style>`,`"+css_pairs+"`)");
console.log("A1=F(`"+html+"`,`"+html_pairs+"`)");
console.log("A2=F(`"+aw+"`,`"+aw_pairs+"`)");
//console.log("eval(F("+JSON.stringify(player)+",`"+player_pairs+"`));");
console.log("eval(F("+sqjson(player)+",`"+player_pairs+"`));");

