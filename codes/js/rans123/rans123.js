// base-123 decoder => rANS decoder

// "base-123" may be slightly misleading, because it converts "tuples" of 17
// base-123 encoded chars into a 118-bit value, so it's only
// base-122.88597591741355 in terms of data density
// (2**(118/17)=122.88597591741355)
// "17" was chosen as to not waste too many fractional bits:
// Math.log2(123**17) = 118.02274659076708
// the next exponent that has a lower fraction is 52:
// Math.log2(123**52)=361.0107542776405
// To see if it makes a difference try changing "17n" to "52n" and "118n" to
// "361n"

// Usage:
//   X=X(14,"your rANS / base-123 encoded data blah blah blah ..");
//   X()            => returns cumulative frequency for current symbol (corresponds to RansDecGet())
//   X(start,freq)) => advance to next symbol (corresponds to RansDecAdvance())
// see RansDecGet()/RansDecAdvance() in https://github.com/rygorous/ryg_rans

X=(scale_bits,input)=>{
	let i,
	byte_buffer = [],
	bit_buffer = [],
	input_index = 0,
	tmp0,
	byt,
	code,
	pull_byte = _=>{
	    	if (!byte_buffer.length) {
			// translate 17 base-123 encoded digits into a 118-bit
			// value which is pushed bit-for-bit onto bit_buffer,
			// which is transferred to byte_buffer, 8 bits at a
			// time
			tmp0 = 0n;
			for (i=0n; i<17n; i++) { // XXX(MAGIC)(stride=17)
				// base-123 encoding: mapping is all ASCII characters
				// in order, except:
				//   NUL (\x00)    0 (technically valid in JS, but not HTML?, and \x00 requires 4 bytes")
				//   LF  (\n)     10 (\n requires 2 bytes)
				//   CR  (\r)     13 (\r requires 2 bytes)
				//   "            34 (\" requires 2 bytes)
				//   \            92 (\\ requires 2 bytes)
				// so 0 is encoded as 1, and 122 is encoded as 127
				code = input.charCodeAt(input_index++);
				tmp0 += BigInt(code-1-(code>10)-(code>13)-(code>34)-(code>92)) * (123n ** i); // XXX(MAGIC)(base=123)
			}
			for (i=0n; i<118n; i++) bit_buffer.push((tmp0 >> i) & 1n); // XXX(MAGIC)(bits/stride=118)
			while (bit_buffer.length > 7) {
				tmp0 = 0;
				for (i=0; i<8; i++) tmp0 |= bit_buffer.shift() ? 1<<i : 0;
				byte_buffer.push(tmp0);
			}
		}
		return byte_buffer.shift();
	},
	mask = (1<<scale_bits)-1,
	rans_state=0
	;

	// rANS decoder
	for(i=0;i<32;i+=8) rans_state |= pull_byte() << i;
	return (start,freq)=>{
		if (start === undefined) return rans_state & mask;
		// carefully rewritten to avoid negative values (e.g. in JS:
		// 1<<31===-2147483648)
		//  - (rans_state>>scale_bits)       =>   ((rans_state/(mask+1))|0)
		//  - (rans_state<<8) | pull_byte()  =>   (rans_state * 256) + pull_byte()
		// NOTE (rans_state & mask) is safe
		rans_state = freq * ((rans_state/(mask+1))|0) + (rans_state & mask) - start;
		while (rans_state < (1<<23)) rans_state = (rans_state * 256) + pull_byte();
	}
}
