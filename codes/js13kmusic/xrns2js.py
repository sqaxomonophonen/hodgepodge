#!/usr/bin/env python
import sys
import os
import xml.etree.cElementTree as et
import json

tree = et.fromstring(sys.stdin.read())

song_data = tree.find('GlobalSongData')
bpm = int(song_data.find('BeatsPerMin').text)
lpb = int(song_data.find('LinesPerBeat').text)
time_signature = int(song_data.find('SignatureNumerator').text) # XXX always 4

sequence = tree.find('PatternSequence').find('SequenceEntries')
patterns = tree.find('PatternPool').find('Patterns')

song = [[],[]]

for e in sequence:
	pi = int(e.find('Pattern').text)
	p = patterns[pi]
	nl = int(p.find('NumberOfLines').text)
	ts = p.find('Tracks').findall('PatternTrack')
	for pti, t in enumerate(ts):
		if pti >= len(song): continue
		track = song[pti]
		last_index = -1
		for line in t.find('Lines') or []:
			index = int(line.get('index'))
			for i in range(index - last_index - 1):
				track.append(0)

			note = line.find('NoteColumns').find('NoteColumn').find('Note').text

			n0 = {
				"C": 0,
				"D": 2,
				"E": 4,
				"F": 5,
				"G": 7,
				"A": 9,
				"B": 11
			}
			n1 = {"-": 0, "#": 1}

			note_value = n0[note[0]] + n1[note[1]] + int(note[2])*12
			track.append(note_value)

			last_index = index
		for i in range(nl - last_index - 1):
			track.append(0)

print "var SONG = %s;" % json.dumps(song)
