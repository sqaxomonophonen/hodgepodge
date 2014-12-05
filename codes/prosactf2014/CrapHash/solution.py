#!/usr/bin/env python
data = open("file.bin").read()
open("attack.bin", "w").write(data[16:32] + data[0:16] + data[32:])
