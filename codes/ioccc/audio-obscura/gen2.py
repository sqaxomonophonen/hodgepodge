def X(s): print(hex(int(s.replace("x","1").replace(".","0")[::-1],2)))
X("x.xx.xx.xx.xxxxxxxxxx.x.x.x.xxxx")
X("xx.xx.xx.x.xx.xxxxx.x.xx.x.xxxxx")
