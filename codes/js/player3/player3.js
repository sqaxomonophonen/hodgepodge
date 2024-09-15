function P(sample_rate,n_samples,callback) { window.onload=_=>{
	let worklet_node, audio_ctx, gain_node, playing;
	let post_worklet_message = message => worklet_node.port.postMessage(message);

	let play_audio_at = position=>{
		position = (position*n_samples)|0;
		if(!audio_ctx) {
			audio_ctx = new AudioContext({sampleRate:sample_rate});
			audio_ctx.audioWorklet.addModule("data:text/javascript;base64,"+btoa(A2)).then(_=>{
				gain_node = audio_ctx.createGain();
				gain_node.connect(audio_ctx.destination);
				worklet_node = new AudioWorkletNode(audio_ctx, "a",{numberOfInputs:0,outputChannelCount:[2]});
				worklet_node.connect(gain_node);
				worklet_node.port.addEventListener("message", (msg) => {
					//document.getElementById("pos").innerHTML = msg.data.position;
					// TODO
				});
				worklet_node.port.start();
				audio_ctx.resume();
				post_worklet_message({p:position});
			});
		} else {
			post_worklet_message({p:position});
		}
		playing=1;
	};

	let stop_audio = _=> {
		playing=0;
		post_worklet_message({s:1});
	};

	let varinject = (s) => s.replace(/\$C/g,"#fb1"),
	    sethtml=(e,s)=>e.innerHTML=s
	    ;

	sethtml(document.head, varinject(A0));
	sethtml(document.body, varinject(A1));

	let songlen = n_samples/sample_rate;
	function animate() {
		window.requestAnimationFrame(_=>{
			let n=Date.now(),
			    t=(n*2e-4)%1,
			    tf=(t*360).toFixed(4),
			    xf=(45+3*Math.sin(n*0.06)).toFixed(0),
			    ps=pp.style;
			ps.background = 'lch('+xf+' 60 '+tf+')';
			ps.width = ((1-((n*7e-5)%1))*100).toFixed(1)+'%';
			animate();
		});
	}

	let fmtmmss = (dur,durmax) => {
		dur = dur|0;
		let mm = ""+((dur/60)|0);
		let ss = ""+(dur-mm*60);
		if (ss.length === 1) ss = "0"+ss;
		let mmx = ""+((durmax/60)|0);
		while (mm.length < mmx.length) mm="0"+mm;
		return mm+":"+ss;
	};

	let clamp = x=>Math.min(1,Math.max(0,x));

	let scrubbing = 0 ,
	    voluming = 0 ,
	    pos ,
	    gain = 1 ,
	    previous_gain = null ,
	    fmtpct = (scalar) => (scalar*100).toFixed(2)+'%' ,

	    set_gain = g=>{
		gain = clamp(g);
		v0.style.width = v1.style.left = fmtpct(gain);
		sp.style.opacity = gain > 0 ? 1 : 0.2;
		for (let i = 0; i < 3; i++) {
			document.getElementById("c"+i).style.opacity = clamp(gain*3-i);
		}
	    },

	    set_pos = p=>{
		pos = clamp(p);
		t3.style.width = fmtpct(pos);
		t2.style.left = fmtpct(pos);
		sethtml(tpos, fmtmmss(pos*songlen,songlen));
		sethtml(tend, fmtmmss(songlen,songlen));
	    },

	    get_element_pos = (em,ev) => {
		const r = em.getBoundingClientRect();
		return (ev.clientX - r.left) / r.width;
	    },

	    scrub = e=>{
		set_pos(get_element_pos(t0,e));
	    },

	    volum = e=>{
		set_gain(get_element_pos(vo,e));
		previous_gain = null;
	    },

	    setmdown = (em,handler)=>em.onmousedown=handler,

	    wheely = _=>{
		set_gain(gain+0.03*(event.deltaY<0?1:-1));
	    },

	    show = (e,p) => e.style.display = p?'':'none';

	    ;

	set_pos(0);
	setmdown(t0,_=>{
		scrubbing = 1;
		scrub(event);
	});
	setmdown(vo,_=>{
		voluming = 1;
		volum(event);
	});
	vo.onwheel = wheely;
	sp.onwheel = wheely;
	window.onmousemove=_=>{
		if (scrubbing) scrub(event);
		if (voluming) volum(event);
	};
	window.onmouseup=_=>{
		if (scrubbing && playing) play_audio_at(pos);
		scrubbing = 0;
		voluming = 0;
	};
	setmdown(sp,_=>{
		if (previous_gain !== null) {
			set_gain(previous_gain);
			previous_gain = null;
		} else if (gain === 0) {
			set_gain(1);
		} else {
			previous_gain = gain;
			set_gain(0);
		}
	});


	setmdown(b1,_=>{
		play_audio_at(pos);
		show(b1,0);
		show(b0,1);
	});

	setmdown(b0,_=>{
		stop_audio();
		show(b1,1);
		show(b0,0);
	});

	animate();

	callback(chunk => {
		console.log(["TODO CHUNK", chunk]);
		// two destinations:
		//  - worklet: post_worklet_message({c:Float32Array...})
		//  - local .wav buffer?
	});
}}
