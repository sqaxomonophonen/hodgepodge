const earth_radius_km = 6371;
const N = 10000000;

const rnd = (max) => Math.random() * max;
/*
let args = [];
for (let i = 0; i < N; i++) {
	args.push(rnd(360)-180);
	args.push(rnd(180)-90);
	args.push(rnd(360)-180);
	args.push(rnd(180)-90);
}
*/
let args = new Float64Array(N*4);
for (let offset = 0; offset < N*4; offset+=4) {
	args[offset+0] = rnd(360)-180;
	args[offset+1] = rnd(180)-90;
	args[offset+2] = rnd(360)-180;
	args[offset+3] = rnd(180)-90;
}

const radians = (degrees) => 0.017453292519943295 * degrees;

const haversine_of_degrees = (x0,y0,x1,y1,r) => {
	const dy = radians(y1-y0);
	const dx = radians(x1-x0);
	const y0r = radians(y0);
	const y1r = radians(y1);
	const syroot = Math.sin(dy * 0.5);
	const sxroot = Math.sin(dx * 0.5);
	const root_term = syroot*syroot + Math.cos(y0r)*Math.cos(y1r)*sxroot*sxroot;
	return 2.0 * r * Math.asin(Math.sqrt(root_term));
};

const N_TRIALS = 10;
let best_rate = 0;
for (let i0 = 0; i0 < N_TRIALS; i0++) {
	let sum = 0.0;
	let offset = 0;
	let t0 = Date.now();
	for (let i = 0; i < N; i++) {
		sum += haversine_of_degrees(args[offset+0], args[offset+1], args[offset+2], args[offset+3], earth_radius_km);
		offset += 4;
	}
	const dt = Date.now() - t0;
	const rate = (N/(dt/1000.0));
	if (rate > best_rate) best_rate = rate;
	console.log("avg is " + (sum/N).toFixed(0) + ". " + (N/(dt/1000.0)).toFixed(2) + " haversines/s");
}
console.log("best: " + best_rate.toFixed(0) + " haversines/s");
