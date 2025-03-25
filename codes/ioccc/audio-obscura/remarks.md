### Instructions
Just make, run `./prog` and wait; it'll tell you what to do.

**SPOILERS AHEAD**

### Obfuscations
 - "Helpful" formatting
 - Character salad
 - Loop exit trying to hide by a switch statement
 - Strong string encryption. By that I mean "IBM"=>"HAL" (and "fmt"=>"gnu")
 - A `/*{*/` that throws off bracket matching in vim. Begun the editor wars
   have! (wait, I'm **on** the vim-team!)
 - `cm` and `ap` stands for "allpass" and "comb", respectively.
 - Deliberate memory "corruption"

To encourage tinkering I've obfuscated the song data and program (look for Y())
less than the "library code". Sorry if you wanted to tinker with the library
code!

### Sound quality
The sample rate of prog.wav is 256kHz because creating aliasing-free waveforms
would require more code, but you can always oversample the audio and pass the
anti-aliasing problem on to the audio player :-) However, mplayer on my
computer uses a poor downsampler. But you can use the high-quality resampler in
sox to create a 48kHz version:
```
    sox prog.wav -r 48000 prog.48kHz.wav
```

### Tricks (the condenscending part)
 - Kick drum from sines, pitched up to create toms.
 - Roland TR-808 style hi-hats (a bunch of high-pass filtered square waves)
 - SID/C64-style snare drum (oscillates between noise and square wave)
 - There's some FM synthesis in the bass
 - Schroeder reverberator a la Freeverb. The same building blocks are used for
   the echo effect.
 - Waveshaping/overdrive using tanh(x), ohh yeaaah..

I used this Python snippet to generate `q0[2]` and `q0[3]`:
```
def X(s): print(hex(int(s.replace("x","1").replace(".","0")[::-1],2))) # not for iopcc-use
X(".x.x..xx.x.xx.xxx.xx.xxx..xx.xxx")
X("x.xx.x.x.x.xxxxxx.xx.xxx.xxxxxxx")
```
