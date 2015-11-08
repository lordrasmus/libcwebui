#!/usr/bin/python3

import os, sys, subprocess

from pprint import pprint

ret = subprocess.getstatusoutput( "find output1/ | grep crashes | grep id")
liste1 = ret[1].split("\n")

ret = subprocess.getstatusoutput( "find input ")
liste2 = ret[1].split("\n")

liste = liste2 + liste1

#pprint( liste )
#sys.exit(1)

for crash in liste:

	print("testing : " + crash)
	ret = subprocess.getstatusoutput( "./main.afl.gcc < " + crash )
	#pprint ( ret )

	if not ret[0] == 0:
		#./main.afl.gcc < " + crash )

		f = open(".gdbinit","w")
		f.write("file ./main\n")
		f.write("r < " + crash )

		f.close()

		os.system("gdb ")

		sys.exit(1)


