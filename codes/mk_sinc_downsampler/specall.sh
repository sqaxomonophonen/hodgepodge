#!/usr/bin/env bash
for wav in *.wav ; do
	echo "$wav -spectrogram-> $wav.png"
	sox $wav -n spectrogram -o $wav.png
done
