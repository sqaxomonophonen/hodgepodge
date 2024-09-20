#!/usr/bin/env node

path=require("path");
fs=require("fs");

if (process.argv.length !== 4) {
	console.log("Usage: "+path.basename(process.argv[1])+" <input> <output>");
	process.exit(1);
}

const input = fs.readFileSync(process.argv[2]);

let bit_buffer = [];
for (let i0 = 0; i0 < input.length; i0++) {
	const b = input[i0];
	for (let i1 = 0; i1 < 8; i1++) bit_buffer.push((b>>i1)&1);
}

let b123map = [];
for (let i = 0; i < 128; i++) {
	if ("\x00\n\r\"\\".indexOf(String.fromCharCode(i)) >= 0) continue;
	b123map.push(i);
}
if (b123map.length !== 123) throw new Error("XXX");

let buffer = new Uint8Array(1<<20);
let bit_cursor = 0;
let byte_cursor = 0;
while (bit_cursor < bit_buffer.length) {
	let val = 0n;
	for (let bit = 0n; bit < 118n; bit++) {
		if (bit_cursor < bit_buffer.length && bit_buffer[bit_cursor]) val |= 1n<<bit;
		bit_cursor++;
	}
	for (let i = 0n; i < 17n; i++) {
		const base = 123n;
		buffer[byte_cursor++] = b123map[val % base];
		val /= base;
	}
}
fs.writeFileSync(process.argv[3], buffer.slice(0,byte_cursor));
console.log("base123enc", input.length, "=>", byte_cursor, "( efficiency:", input.length/byte_cursor, ")");
