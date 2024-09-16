function P(sample_rate, n_channels, n_frames, song_text, main_color) {
	let prebuf=[],
	    post_worklet_message,
	    n_chunks_generated = 0,
	    chunk_frame_length = 0,
	    clampmm = (x,min,max)=>Math.min(max,Math.max(min,x)),
	    clamp = x=>clampmm(x,0,1)
	    ;
	window.onload=_=>{
		let worklet_node, audio_ctx, gain_node, playing, set_pos,
		    gain = 1 ,
		    set_gain_node_gain = _=>{
			    if (gain_node) gain_node.gain.value = (Math.pow(3,gain)-1)/(3-1);
		    },

		    play_audio_at = position=>{
			position = (position*n_frames)|0;
			if(!audio_ctx) {
				audio_ctx = new AudioContext({sampleRate:sample_rate});
				audio_ctx.audioWorklet.addModule("data:text/javascript;base64,"+btoa(A2)).then(_=>{
					gain_node = audio_ctx.createGain();
					gain_node.connect(audio_ctx.destination);
					set_gain_node_gain();
					worklet_node = new AudioWorkletNode(audio_ctx, "a",{numberOfInputs:0,outputChannelCount:[n_channels]});
					worklet_node.connect(gain_node);
					worklet_node.port.onmessage = msg => {
						set_pos(msg.data.p / n_frames);
					};
					worklet_node.port.start();
					audio_ctx.resume();
					post_worklet_message = message => worklet_node.port.postMessage(message);
					if (prebuf) {
						post_worklet_message({c:prebuf});
						prebuf=0;
					}
					post_worklet_message({p:position});
				});
			} else {
				post_worklet_message({p:position});
			}
			playing=1;
		    },

		    stop_audio = _=> {
			playing=0;
			post_worklet_message({s:1});
		    },

		    varinject = (s) => s.replace(/\$C/g,main_color),

		    sethtml=(e,s)=>e.innerHTML=s

		    ;

		sethtml(document.head, varinject(A0));
		sethtml(document.body, varinject(A1));

		st.innerHTML = song_text;

		let songlen = n_frames/sample_rate;
		function animate() {
			window.requestAnimationFrame(_=>{
				if (!window.pp) return;
				let n=Date.now(),
				    t=(n*2e-4)%1,
				    tf=(t*360).toFixed(4),
				    xf=(45+3*Math.sin(n*0.06)).toFixed(0),
				    ps=pp.style;
				ps.background = 'lch('+xf+' 60 '+tf+')';
				//ps.width = ((1-((n*7e-5)%1))*100).toFixed(1)+'%';
				ps.width = (clamp(1-((n_chunks_generated * chunk_frame_length) / n_frames))*100).toFixed(1)+"%";
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

		set_playing = p=>{
			if (p) {
				if (pos==1) pos=0;
				play_audio_at(pos);
			} else {
				stop_audio();
			}
			show(b1,p^1);
			show(b0,p);
		},

		set_pos = p=>{
			pos = clamp(p);
			if (pos==1) set_playing(0);
			t3.style.width = fmtpct(pos);
			t2.style.left = fmtpct(pos);
			sethtml(tpos, fmtmmss(pos*songlen,songlen));
			sethtml(tend, fmtmmss(songlen,songlen));
		};

		let scrubbing = 0 ,
		    voluming = 0 ,
		    pos ,
		    previous_gain = null ,
		    fmtpct = (scalar) => (scalar*100).toFixed(2)+'%' ,

		    set_gain = g=>{
			gain = clamp(g);
			set_gain_node_gain();
			v0.style.width = v1.style.left = fmtpct(gain);
			sp.style.opacity = gain > 0 ? 1 : 0.2;
			for (let i = 0; i < 3; i++) {
				document.getElementById("c"+i).style.opacity = clamp(gain*3-i);
			}
		    },

		    get_element_pos = (em,ev) => {
			let r = em.getBoundingClientRect();
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
			set_playing(0);
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

		setmdown(b1,_=>set_playing(1));
		setmdown(b0,_=>set_playing(0));

		animate();
	}

	let data_length = n_frames * n_channels * 2,
	    wave_length = 44+data_length,
	    WAVE = new Uint8Array(wave_length),
	    cursor = 0,
	    wave_url,
	    wu16 = value => { WAVE[cursor++]=value; WAVE[cursor++]=value>>8; },
	    wu32 = value => { wu16(value); wu16(value>>16); },
	    wstr = s => { for (s of s) WAVE[cursor++]=s.charCodeAt(0); }
	    ;

	wstr("RIFF");
	wu32(data_length+36);
	wstr("WAVEfmt ");
	wu32(16);
	wu16(1);
	wu16(n_channels);
	wu32(sample_rate);
	wu32(sample_rate * n_channels * 2);
	wu16(n_channels * 2);
	wu16(16);
	wstr("data");
	wu32(data_length);

	return chunk => {
		chunk_frame_length = chunk.length / n_channels;
		n_chunks_generated++;
		if (prebuf) {
			prebuf.push(chunk);
		} else {
			post_worklet_message({c:chunk});
		}
		for (let i = 0; i < chunk.length; i++) {
			wu16(clampmm(chunk[i],-1,1)*32767 + Math.random()*0.5);
		}
		if ((cursor-wave_length) === 0) {
			wave_url = URL.createObjectURL(new File([WAVE],'my_song.wav',{'type':'audio/wav'}));
			dl0.style.display = '';
			dl1.onclick = _=>{
				p0.innerHTML = '<audio controls src="' + wave_url + '">';
			};
		}
	};
}
