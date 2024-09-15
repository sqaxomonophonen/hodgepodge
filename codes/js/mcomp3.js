fs=require('fs');

const SPLIT="Z";

function COMP(path, prefixes) {
	const orig = fs.readFileSync(path,'utf-8');
	let w = orig;
	w = w.replaceAll("\n","");
	let next_token = (() => {
		//let single_letter = "ABCDEFGHIJKLMNOPQRSTUVWXZY";
		let single_letter = "ABCDEFGHIJKLMNOPQRSTUVWXY";
		let is_valid = (token) => w.indexOf(token) === -1;
		if (!is_valid(SPLIT)) throw new Error("source contains SPLIT char; please remove it, or change SPLIT");
		let isl = 0;
		return () => {
			for (;;) {
				if ((isl++) < single_letter.length) {
					const token = single_letter[isl-1];
					if (is_valid(token)) return token;
				} else {
					throw new Error("TODO other schemes?");
				}
			}
		};
	})();
	let token = next_token();
	let pairs = [];
	let ratio;
	let prev_ratio = 1;
	const get_ratio = _=>(w.length + 5 + pairs.join(SPLIT).length) / orig.length;

	for (let prefix of prefixes) {
		pairs.push(token)
		pairs.push(prefix);
		w=w.replaceAll(prefix,token);
		ratio = get_ratio();
		if (ratio > prev_ratio) {
			console.error("WARNING: prefix does not seem to compress: " + prefix)
			console.error("Bad ratio change:", prev_ratio, "=>", ratio);
		}
		prev_ratio = ratio;
		token = next_token();
	}
	console.error(path, "compression:", ratio, "( without dictionary:" , w.length/orig.length, ")");
	return [w, pairs.join(SPLIT)];
}

let [ css, css_pairs ] = COMP("player3.css",[
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
	"0 0 ",
	"px",
]);

let [ html, html_pairs ] = COMP("player3.doc.html",[
	'<path stroke="$C" stroke-width="1" fill="none" id="c',
	'<rect width="6" height="16" y="2" style="fill:$C" x="',
	'<polygon points="',
	'<svg viewBox="0 0 ',
	'<div class="',
	'<div id="',
	'style="',
	'class="',
	"</div>",
	'width="',
	'height="',
	"</svg>",
	"0 0 ",
]);

let [ aw, aw_pairs ] = COMP("worklet3.min.js",[
	'Processor',
	'.length',
	'this.'
]);

html = html.replace(/<!--[\s\S]*?-->/g, ''); // remove HTML comments
css  = css.replace(/\/\*[\s\S]*?\*\//g, ''); // remove CSS comments

console.log("F=(s,p)=>{p=p.split('"+SPLIT+"');while(p.length){let x=p.pop();s=s.replaceAll(p.pop(),x);}return s}");
console.log("A0=F(`<style>"+css+"</style>`,`"+css_pairs+"`)");
console.log("A1=F(`"+html+"`,`"+html_pairs+"`)");
console.log("A2=F(`"+aw+"`,`"+aw_pairs+"`)");

