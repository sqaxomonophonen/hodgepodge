fs=require('fs');

const SPLIT="Z";

function COMP(path, prefixes) {
	const orig = fs.readFileSync(path,'utf-8');
	let w = orig;
	let next_token = (() => {
		//let single_letter = "ABCDEFGHIJKLMNOPQRSTUVWXZY";
		let single_letter = "ABCDEFGHIJKLMNOPQRSTUVWXY";
		let is_valid = (token) => w.indexOf(token) === -1;
		if (!is_valid(SPLIT)) throw new Error("XXX");
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
	for (let prefix of prefixes) {
		pairs.push(token)
		pairs.push(prefix);
		w=w.replaceAll(prefix,token);
		token = next_token();
	}
	w = w.replaceAll("\n","");
	pairs = pairs.join(SPLIT);
	const ratio = (w.length + pairs.length) / orig.length;
	console.error(path, "compression:", ratio, w.length/orig.length);
	return [w, pairs];
}

let [ css, css_pairs ] = COMP("player3.css",[
	"filter:drop-shadow(",
	"px #000",
	"0 0 ",
	"px",
	"background",
	"relative",
	"position",
	"width",
	"height",
	"margin",
	"color",
	"left:",
	"right:",
]);

let [ html, html_pairs ] = COMP("player3.doc.html",[
	'<path stroke="$C" stroke-width="1" fill="none" id="vbar',
	'<rect width="6" height="16" y="2" style="fill:$C" x="',
	'<polygon points="',
	'viewBox="0 0 ',
	'<div id="',
	'<div class="',
	'style="',
	'class="',
	"</div>",
	'width="',
	'height="',
	"svg",
	"0 0 ",
]);

console.log("F=(s,p)=>{p=p.split('"+SPLIT+"');while(p.length){let x=p.pop();s=s.replaceAll(p.pop(),x);}return s}");
console.log("HEAD=F(`<style>"+css+"</style>`,"+JSON.stringify(css_pairs)+")");
console.log("HTML=F(`"+html+"`,"+JSON.stringify(html_pairs)+")");

