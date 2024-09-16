(_=>{
	let SAMPLE_RATE=44100,
	    N_CHANNELS=2,
	    CHUNK_FRAMES=SAMPLE_RATE/10,
	    N_CHUNKS=100,
	    push=P(SAMPLE_RATE, N_CHANNELS, N_CHUNKS*CHUNK_FRAMES, "song3 - aks", "#fb1"),
	    remaining=N_CHUNKS,
	    phi_left=0,
	    phi_right=0,
	    dphi_left = 0.1,
	    dphi_right = 0.11,
	    X
	    ;

	X = () => {
		if (remaining-- <= 0) return;
		let xs = new Float32Array(CHUNK_FRAMES*N_CHANNELS);
		let p = 0;
		for (let i = 0; i < CHUNK_FRAMES; i++) {
			xs[p++] = 0.9 * Math.sin(phi_left);
			xs[p++] = 0.9 * Math.sin(phi_right);
			phi_left += dphi_left;
			phi_right += dphi_right;
			dphi_left += 1.1e-7;
			dphi_right += 1e-7;
			while (phi_left > 2*Math.PI) phi_left -= 2*Math.PI;
			while (phi_right > 2*Math.PI) phi_right -= 2*Math.PI;
		}
		push(xs);
		setTimeout(X,10);
	}
	X();
})();
