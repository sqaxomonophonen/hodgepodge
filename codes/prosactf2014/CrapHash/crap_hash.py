#!/usr/bin/python

from Crypto.Cipher import AES
import sys

def xor_strings(s1, s2):
    return ''.join([chr(ord(s1[i]) ^ ord(s2[i])) for i in range(len(s1))])

def to_hex(s):
    return ''.join(['%02x' % ord(c) for c in s])

def hash_file(f):
    h = '\0' * 16
    aes = AES.new(h, AES.MODE_ECB)
    has_padded = False
    while True:
        d = f.read(16)
        if not d:
            break
        if not len(d) == 16:
            d += '\x80'
            d += '\x00' * (16 - len(d))
            has_padded = True
        c = aes.encrypt(d)
        h = xor_strings(h, c)

    if not has_padded:
        c = aes.encrypt('\x80' + '\x00' * 15)
        h = xor_strings(h, c)

    return h

for s in sys.argv[1:]:
    with open(s) as f:
        print to_hex(hash_file(f)), s
