
// A() decodes base64 to u8 array (not Uint8Array)
A=s=>atob(s).split('').map(x=>x.charCodeAt()) // 54ch
//234567890123456789012345678901234567890123456789
//        |         |         |         |         |
//       10        20        30        40        50

// B() decodes base64 as u6-based array of variable size integers; values are
// big-endian; the high bit (32) is set on the last (and least significant)
// digit. examples:
//   B("g")    => [0]
//   B("gg")   => [0,0]
//   B("ggBg") => [0,0,64]
//   B("ggBh") => [0,0,65]
//   B("BAg")  => [4096]
// The base64 alphabet is A-Z,a-z,0-9,+,/
B=s=>{let r=[],v=0;for(s of s.split('').map(x=>(x=x.charCodeAt(),x==43?62:x==47?63:x<58?x+4:x<91?x-65:x-71))){v+=s&31;s&32?(r.push(v),v=0):v*=64}return r} // 153ch
//23456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
//        |         |         |         |         |         |         |         |         |         |         |         |         |         |         |         |
//       10        20        30        40        50        60        70        80        90       100       110       120       130       140       150       160
B=s=>{let r=[],v=0;for(s of s){s=s.charCodeAt();s=s==43?62:s==47?63:s<58?s+4:s<91?s-65:s-71;v+=s&31;s&32?(r.push(v),v=0):v*=64}return r} // 135ch
//2345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
//        |         |         |         |         |         |         |         |         |         |         |         |         |         |
//       10        20        30        40        50        60        70        80        90       100       110       120       130       140




// $() decodes a list of string symbols, where:
//  - multi-character symbols are quoted in '$', e.g. '$foo$' => ['foo']
//  - otherwise, characters are individual 1-character symbols, e.g. 'bar' => ['b','a','r']
//  - '$' itself is encoded as '$$'
//  - '$' in multi-character symbols is not possible
// examples:
//   $('bar')      =>  ['b','a','r']
//   $('$foo$bar') =>  ['foo','b','a','r']
$=(s)=>{let i=0,r=[];for(s of s.split('$')){if((i++)&1){r.push(s?s:'$');}else{r=r.concat(s.split(''))}}return r} // 111ch
//23456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
//        |         |         |         |         |         |         |         |         |         |         |         |
//       10        20        30        40        50        60        70        80        90       100       110       120
$=(s)=>{let p=0,r=[];for(s of s.split('$')){if(p=!p){r=r.concat(s.split(''))}else{r.push(s?s:'$')}}return r} // 107ch
$=(s)=>{let p=0,r=[];for(s of s.split('$')){(p=!p)?r=r.concat(s.split('')):r.push(s?s:'$')}return r} // 99ch
//2345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
//        |         |         |         |         |         |         |         |         |         |         |
//       10        20        30        40        50        60        70        80        90       100       110





// rans decoder
//  - data: rans-encoded data array (either [0;255]-array or Uint8Array)
//  - n_dec: number of symbols in decode stream
//  - prob_bits: number of probability bits, e.g. 14
//  - probs: symbol probability array
//  - syms: symbol array
// returns array of decoded symbols
D=(data,n_dec,prob_bits,probs,syms)=>{
	let max=1<<prob_bits,mask=max-1,p,lut=[],start=[],li=0;
	for (let i0 in probs) {
		p = probs[i0];
		start.push(li);
		for (let i1 = 0; i1 < p; i1++) {
			lut[li++] = i0;
		}
	}
	if (li !== max || lut.length !== max) throw new Error("bad symbol probability array; must add up to 1<<"+prob_bits+"="+max+" but got "+li);

	let st=0,M=256,P8=()=>data.shift(),symidx,r=[];
	st  = P8();
	st += P8()*M;
	st += P8()*M*M;
	st += P8()*M*M*M;

	for (let i = 0; i < n_dec; i++) {
		symidx = lut[st & mask]
		r.push(syms[symidx]);
		st = probs[symidx] * ((st/max)|0) + (st&mask) - start[symidx];
		while (st < (1<<23)) st = st*M + P8();
		/*
		if (st < (1<<23)) {
			do {
				st = st*256 + P8()
			} while (st < (1<<23));
		}
		*/
	}

	console.log(r);
	return r;
};

D(A("ey6fgliDteqjPzSwey6fgliDteqjPzSw"),10,14,[1<<13,1<<12,1<<12],["a","b","c"]);






///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

function TEST(name, fn) {
	let equals; equals = (a,b) => {
		const a_is_array=(a instanceof Array), b_is_array=(b instanceof Array);
		if (a_is_array && b_is_array) {
			if (a.length !== b.length) return false;
			for (let i=0; i<a.length; i++) {
				if (!equals(a[i],b[i])) return false;
			}
			return true;
		}

		const a_is_object=(a instanceof Object), b_is_object=(b instanceof Object);
		if (a_is_object || b_is_object) throw new Error("cannot compare objects");

		return a===b;
	};
	fn({
		assert: (p,msg) => {
			if (!p) throw new Error("ASSERTION FAILED"+(msg?": "+msg:""));
		},
		equals: equals,
		expect: (expected,actual,msg) => {
			if (!equals(expected,actual)) throw new Error("expected "+JSON.stringify(expected)+"; but got "+JSON.stringify(actual));
		},
	});
	console.log("TEST " +name+": OK");
}



TEST("test-test", (t) => {
	t.assert(t.equals(420,420));
	t.assert(t.equals([1,2,3],[1,2,3]));
	t.assert(t.equals([1,2,[3,4]],[1,2,[3,4]]));
	t.expect(420,420);
	t.expect([1,2,3],[1,2,3]);
	t.expect([1,2,[3,4]],[1,2,[3,4]]);
});

TEST("B()", (t) => {
	t.expect([ 4096 ],     B("BAg"));
	t.expect([ 0, 0 ],     B("gg"));
	t.expect([ 0, 0, 64 ], B("ggBg"));
	t.expect([ 0, 0, 64, 30, 31 ], B("ggBg+/"));
});

TEST("$()", (t) => {
	t.expect([ 'b','a','r' ],     $("bar"));
	t.expect([ 'foo', 'b','a','r' ],     $("$foo$bar"));
	t.expect([ 'foo', 'b','a','r','$' ],     $("$foo$bar$$"));
	t.expect([ 'foo', 'quux', 'b','a','r','$' ],     $("$foo$$quux$bar$$"));
});
