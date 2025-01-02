

#for K in range(1,10):
for K in range(1,2):
	def enc(s):
		u0="".join([chr(ord(c)^K) for c in s])
		u1="".join([chr(ord(c)+K) for c in s])
		#print("%s -> %s (^%d)" % (repr(s),repr(u0),K))
		print("%s -> %s (+%d)" % (repr(s),repr(u1),K))

	enc("RIFF")
	enc("WAVE")
	enc("fmt ")
	enc("data")
	enc("play ")
	enc("enjoy ")
	enc(".wav")
	enc("song")
	print()
