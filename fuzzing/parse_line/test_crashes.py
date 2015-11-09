#!/usr/bin/python3

import os, sys, subprocess

from pprint import pprint

os.system("make main")

ret = subprocess.getstatusoutput( "find output*/ | grep crashes | grep id")
liste1 = ret[1].split("\n")

ret = subprocess.getstatusoutput( "find input ")
liste2 = ret[1].split("\n")

liste = liste2 + liste1

#pprint( liste )
#sys.exit(1)

for crash in liste:

	if crash == "":
		continue

	print("testing : " + crash)
	ret = subprocess.getstatusoutput( "./main < " + crash )
	#pprint ( ret )

	if ret[0] == 0:
		if len( ret[1] ) > 0 :
			print( ret[1] )
		#pass
	else:
		#./main.afl.gcc < " + crash )

		pprint( ret )

		print("\n---------- Input ------------\n")
		f = open( crash , "rb")
		tmp = f.read()
		pprint( tmp )
		f.close()
		print("\n----------------------\n")

		f = open(".gdbinit","w")
		f.write("file ./main\n")
		f.write("r < " + crash )

		f.close()

		os.system("ddd ")

		sys.exit(1)


