### Instructions
Try `make try`.

### Obfuscations
 - "Helpful" formatting
 - Character salad
 - Loop exit trying to hide by a switch statement
 - Strong string encryption. By that I mean "fmt"=>"gnu".
 - A `/*{*/` that throws off bracket matching in vim. Begun the editor wars
   have! (Wait, I'm **on** the vim-team!)
 - `cm` and `ap` stands for "allpass" and "comb", respectively.
 - One effect was created with deliberate "memory corruption".

To encourage tinkering I've obfuscated the song data and program (see Y()) less
than the "library code". Sorry if you wanted to tinker with the library code!

### Sound quality
The sample rate of prog.wav is 256kHz because creating aliasing-free waveforms
would require more code, but you can always oversample the audio and pass the
anti-aliasing problem on to the audio player :-) However, the downsampler in my
mplayer is poor, but you can use the high-quality resampler in sox to create a
48kHz version:
```
    sox prog.wav -r 48000 prog.48kHz.wav
```

### Tricks (the condescending part)
 - Kick drum from sines, pitched up to create toms.
 - Roland TR-808 style hi-hats (a bunch of high-pass filtered square waves).
 - SID/C64-style snare drum (oscillates between noise and square wave).
 - There's some FM synthesis in the bass.
 - Schroeder reverberator a la Freeverb. The same building blocks are used for
   the echo effect.
 - Waveshaping/overdrive using tanh(x), ohh yeaaah..

I used this Python snippet to generate `q0[2]` and `q0[3]`:
```
def X(s): print(hex(int(s.replace("x","1").replace(".","0")[::-1],2))) # not for iopcc-use
X(".x.x..xx.x.xx.xxx.xx.xxx..xx.xxx")
X("x.xx.x.x.x.xxxxxx.xx.xxx.xxxxxxx")
```
