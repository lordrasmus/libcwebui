#!/usr/bin/python3

import os, sys, subprocess

from pprint import pprint

def run_tester():

	#os.system("make")

	ret = subprocess.getstatusoutput( "find output*/ | grep crashes | grep id")
	liste1 = ret[1].split("\n")

	ret = subprocess.getstatusoutput( "find output*/ | grep hangs | grep id")
	liste2 = ret[1].split("\n")

	#ret = subprocess.getstatusoutput( "find input/ ")
	#liste3 = ret[1].split("\n")

	#liste = liste3 + liste2 + liste1
	liste = liste2 + liste1

	#pprint( liste )
	#sys.exit(1)

	for crash in liste:

		if crash == "":
			continue

		if crash == "input/":
			continue

		print("testing : " + crash)
		ret = subprocess.getstatusoutput( "./main.gcc < " + crash )
		#pprint ( ret )

		if ret[0] == 0:
			if len( ret[1] ) > 0 :
				print( ret[1] )
			#pass
		else:
			#./main.afl.gcc < " + crash )

			print( ret[1] )

			print("\n---------- Input ------------\n")
			f = open( crash , "rb")
			tmp = f.read()
			pprint( tmp )
			f.close()
			print("\n----------------------\n")

			f = open(".gdbinit","w")
			f.write("file ./main.gcc\n")
			f.write("r < " + crash )

			f.close()

			os.system("ddd ")
			#os.system("adg ")

			sys.exit(1)

