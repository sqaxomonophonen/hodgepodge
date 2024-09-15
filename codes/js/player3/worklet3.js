registerProcessor("a", class extends AudioWorkletProcessor {
	constructor() {
		super();
		this.c = []; // chunks
		this.p = 0; // position
		this.s = 0; // state
		this.port.onmessage = ev => {
			ev = ev.data;
			if (ev.s) {
				// stop
				this.s = 0;
			} else if (ev.p !== undefined) {
				// play
				this.s = 1;
				this.p = ev.p;
			} else if (ev.c) {
				// chunk
				this.c.push(ev.data.pcm);
			}
		};
	}

	process(inputs, outputs, parameters) {
		outputs = outputs[0];
		let n_frames = outputs[0].length,
		    n_channels = outputs.length,
		    position = n_channels * this.p,
		    chunks = this.c,
		    n_chunks = chunks.length,
		    chunk_length = chunks[0],
		    is_playing = this.s==1,
		    sample,i0,i1
		    ;

		for (i0 = 0; i0 < n_frames; i0++) {
			is_playing &= position < n_chunks*chunk_length;
			for (i1 = 0; i1 < n_channels; i1++) {
				sample = 0;
				if (is_playing) {
					sample = chunks[(position / chunk_length)|0][position % chunk_length];
					position++;
				}
				outputs[i1][i0] = sample;
			}
		}
		if (position !== this.p) {
			position = (position/n_channels)|0;
			this.p = position;
			this.port.postMessage({p:position});
		}
		return true;
	}
});
