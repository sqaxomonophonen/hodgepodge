const GifReader = require("omggif").GifReader;
const fs = require("fs");

let statements = [];
function add(stmt) {
	statements.push(stmt + " // codegen'd by packart.js");
}

class Img {
	constructor(width, height, data, type) {
		this.width = width;
		this.height = height;
		this.data = data;
		this.get_pixel = this["get_pixel_" + type];
		if (this.get_pixel === undefined) throw new Error("unhandled type: " + type);
	}

	get_pixel_rgba(x,y) {
		const b = x*4 + y*4*this.width;
		let pixel = [0,0,0];
		for (let i = 0; i < 3; i++) pixel[i] = this.data[b+i];
		return pixel;
	}
}

async function load_gif(path) {
	const fh = await fs.promises.open(path, 'r');
	const body = await fh.readFile();
	await fh.close();
	return [new GifReader(body), body]
}

async function load_gif_rgba(path) {
	const [ gif, _ ] = await load_gif(path);
	const data = new Uint8Array(4*gif.width*gif.height);
	gif.decodeAndBlitFrameRGBA(0, data);
	return new Img(gif.width, gif.height, data, "rgba");
}

async function load_gif_palette(path) {
	const [ gif, buf ] = await load_gif(path);
	const info = gif.frameInfo(0);
	let p = info.palette_offset;
	let palette = [];
	for (let i0 = 0; i0 < info.palette_size; i0++) {
		let color = [];
		for (let i1 = 0; i1 < 3; i1++) {
			color.push(buf[p++]);
		}
		palette.push(color);
	}
	return palette;
}

function make_lowbase_string(data) {
	const n = data.length;
	let lowbase_string = new Uint8Array(n);
	const offset = "0".charCodeAt(0); // XXX try "#", " ", etc... see what packs best? ("0" packs better than "#" atm)
	for (let i = 0; i < n; i++) {
		const cc = offset + data[i];
		lowbase_string[i] = cc;
	}
	return Buffer.from(lowbase_string).toString();
}

function make_bit_string(data) {
	const n = data.length;
	let bits = new Uint8Array(n*8);
	let bp = 0;
	for (let i0 = 0; i0 < n; i0++) {
		const v = data[i0];
		for (let i1 = 0; i1 < 8; i1++) {
			bits[bp++] = (v>>i1)&1;
		}
	}
	return make_lowbase_string(bits);
}

async function do_fnt(path, identifier) {
	const fnt = await load_gif_rgba(path);

	const glyph_width = 8;
	const glyph_height = 8;

	const get_pixel = (x,y) => fnt.get_pixel(x,y);

	const is_glyph_pixel = (x,y) => {
		let p = get_pixel(x,y);
		return p[0] > 200 && p[1] > 200 && p[2] > 200;
	};

	let enc = [null, glyph_width, glyph_height];

	let bs = "";

	const font_extract = (x0, y0, chars) => {
		const c0 = chars.charCodeAt(0);
		const c1 = chars.charCodeAt(1);
		const n = (c1 - c0) + 1;
		enc.push(c0);
		enc.push(c1);

		const glyph_run = new Uint8Array(n*glyph_height);
		let gp = 0;
		for (let i = 0; i < n; i++) {
			const c = c0+i;
			const x = x0 + i*glyph_width;
			if (glyph_width !== 8) throw new Error("this code doesn't work");
			for (let dy = 0; dy < glyph_height; dy++) {
				let v = 0;
				for (let dx = 0; dx < glyph_width; dx++) {
					if (is_glyph_pixel(x+dx, y0+dy)) {
						v |= 1<<dx;
					}
				}
				glyph_run[gp++] = v;
			}
		}
		bs += make_bit_string(glyph_run);
	};

	let xc = 0;
	let yc = 0;
	font_extract(xc, yc, "AZ");
	yc += 8;
	font_extract(xc, yc, "az");
	yc += 8;
	font_extract(xc, yc, "09");
	yc += 8;
	font_extract(xc, yc, "\x01\x06");
	yc += 8;

	enc[0] = bs;

	add("const " + identifier + "=" + JSON.stringify(enc) + ";");
}

async function do_pal(path, identifier, n) {
	let pal = await load_gif_palette(path);
	let pa = new Uint8Array(n*3);
	let pp = 0;
	for (let i = 0; i < n; i++) {
		for (let j = 0; j < 3; j++) pa[pp++] = pal[i][j]
	}
	const enc = Buffer.from(pa).toString('base64');
	add("const " + identifier + " = " + JSON.stringify(enc) + ";");
}


async function load_gif_with_sprite_set(path) {
	const img = await load_gif_rgba(path);
	const MARGIN = 8;

	const is_F0F = (pix) => (pix[0] === 255 && pix[1] === 0 && pix[2] === 255);

	let xticks = [];
	let yticks = [];
	for (let axis = 0; axis < 2; axis++) {
		const len = axis === 0 ? img.width : img.height;
		const axis_ticks =  axis === 0 ? xticks : yticks;
		const sx = axis === 0 ? 1 : 0;
		const sy = axis === 0 ? 0 : 1;
		const tx = axis === 0 ? 0 : 1;
		const ty = axis === 0 ? 1 : 0;
		for (let s = MARGIN; s < len; s++) {
			let is_tick = false;
			for (let t = 0; t < MARGIN; t++) {
				const pix = img.get_pixel(s*sx + t*tx, s*sy + t*ty);
				if (is_F0F(pix)) {
					is_tick = true;
					break;
				}
			}
			if (is_tick) axis_ticks.push(s);
		}
	}

	let sprite_set = [];

	for (let yi = 0; yi < yticks.length-1; yi++) {
		for (let xi = 0; xi < xticks.length-1; xi++) {
			const x0 = xticks[xi] + 1;
			const y0 = yticks[yi] + 1;
			if (!is_F0F(img.get_pixel(x0-1,y0-1))) continue;
			//console.log("found sprite at " + JSON.stringify([xi,yi,x0,y0]));
			const x9 = xticks[xi+1];
			const y9 = yticks[yi+1];

			let x1 = undefined,y1 = undefined;
			for (let yj = y0; yj < y9 && x1 === undefined; yj++) {
				for (let xj = x0; xj < x9 && x1 === undefined; xj++) {
					if (is_F0F(img.get_pixel(xj,yj))) {
						x1 = xj;
						y1 = yj;
					}
				}
			}
			if (x1 === undefined || y1 === undefined) throw new Error("unbounded sprite at " + JSON.stringify([x0-1,y0-1]));

			let anchor_y = undefined;
			for (let yj = y0; yj < y9; yj++) {
				if (!is_F0F(img.get_pixel(x0-1,yj))) continue;
				anchor_y = yj-y0;
				break;
			}

			let anchor_x = undefined;
			for (let xj = x0; xj < x9; xj++) {
				if (!is_F0F(img.get_pixel(xj,y0-1))) continue;
				anchor_x = xj-x0;
				break;
			}

			let anchor = [0,0];
			if (anchor_x !== undefined && anchor_y !== undefined) {
				anchor = [anchor_x, anchor_y];
			}

			sprite_set.push({
				index: [xi,yi],
				position: [x0,y0],
				size: [x1-x0, y1-y0],
				anchor: anchor,
			});
		}
	}

	return [ img, sprite_set ];
}

async function do_sprite_set(path, identifier, enc_pix_fn) {
	const [ img, sprite_set ] = await load_gif_with_sprite_set(path);
	let enc = [null];
	let bs = "";
	for (const sprite of sprite_set) {
		const [ xi, yi ]  = sprite.index;
		if (xi < 0 || xi > 0xf || yi < 0 || yi > 0xf) throw new Error("index out of range");
		const index = xi + (yi<<4);

		enc.push(index);
		const [w,h] = sprite.size;
		enc.push(w);
		enc.push(h);
		enc.push(sprite.anchor[0]);
		enc.push(sprite.anchor[1]);
		const data = new Uint8Array(w*h);
		let p = 0;
		const [x0,y0] = sprite.position;
		for (let y = 0; y < h; y++) {
			for (let x = 0; x < w; x++) {
				data[p++] = enc_pix_fn(img.get_pixel(x0+x,y0+y));
			}
		}
		bs += make_lowbase_string(data);
	}
	enc[0] = bs;
	add("const " + identifier + "=" + JSON.stringify(enc) + ";");
}

async function do_3col_sprite_set(path, identifier) {
	await do_sprite_set(path, identifier, (p) => {
		if (p[0] === 0 && p[1] === 0 && p[2] === 0) return 0; // black=>transparent
		if (p[0] > 200 && p[1] > 200 && p[2] > 200) return 1; // light
		return 2; // dark
	});
}

async function run() {
	await do_pal("pal.gif", "PAL", 8);
	await do_fnt("fnt.gif", "FONT0");
	await do_3col_sprite_set("pieces.gif", "PIECES");
}

run().then(() => {
	statements.push("");
	let all = statements.join("\n");
	const dst = "codegen-packed-art.js";
	fs.writeFileSync(dst, all);
	console.log("wrote " + dst);
});
