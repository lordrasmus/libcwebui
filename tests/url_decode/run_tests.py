#!/usr/bin/python3

import os, re, sys, argparse, subprocess

from pprint import pprint

def question_yes_no(text, current):
	"""Auf der Console nach j / n Fragen
	current = "True" Enter ist j
	current = "False" Enter ist n"""
	print ("")
	while True:
		if current == "True":
			ret = input(text + " ( j / n / Enter=j ) " )
		else:
			ret = input(text + " ( j / n / Enter=n ) " )
		#print ret
		if ret == "":
			return current
		if ret == "j":
			return "True"
		if ret == "n":
			return "False"

def get_header_text( text ):
    regex=r'.*-----------  Header Info Start  ----------------------------(.*)-----------  Header Info End  ----------------------------.*'
    l = ( "error" )
    m = re.match(regex, text, flags=re.DOTALL)
    if m: l = m.groups()

    return l[0]


def run_tests():

	parser = argparse.ArgumentParser(description="Execute Header Parser Tests")
	parser.add_argument('--verbose', help='Verbose Error Reporting', action='store_true')
	args = parser.parse_args()

	os.system("make -j 2")
	os.system("mkdir -p results")

	ret = subprocess.getstatusoutput( "find ../../fuzzing/url_decode/input/* | grep -v README.txt")
	liste = ret[1].split("\n")


	#pprint( liste )

	for test in liste:

		sys.stderr.write("run test : " + test + " \x1b[60G -> ")
		sys.stderr.flush()

		ret = subprocess.getstatusoutput("./main.gcc < " + test )
		if not ret[0] == 0:
			print("Error: Parser Fehler\n" + ret[1] )
			continue

		result_file = "results/" + os.path.basename( test )

		if not os.path.exists( result_file ):
			print("Error: keine Result Datei -> %s\n" % result_file)
			print("<---------- Input ---------->")
			os.system("cat " + test )
			print("<---------- Ret Text ------->")
			print(ret[1])
			print("<--------------------------->")

			ans = question_yes_no("Result File erzeugen ?","j")
			if ans == "True":
				f = open( result_file, "w")
				f.write( ret[1] )
				f.close()
				print("\nResult file erzeugt\n")

			continue

		f = open( result_file, "r" )
		text = f.read()
		f.close()

		ret_text = get_header_text( ret[1] )
		soll_text = get_header_text( text )

		if ret_text == "error":
			print("Error: parsing Test Return Text\n")
			continue

		if soll_text == "error":
			print("Error: parsing Test Soll Text\n")
			continue

		if ret_text == soll_text:
			print("\x1b[32mpass\x1b[0m")
		else:
			print("\x1b[31mfail\x1b[0m")

			if args.verbose is True:
				print("< Input >")
				os.system("cat " + test )
				print("< Ret Text >")
				print(ret_text)
				print("< Soll Text >")
				print(soll_text)
				print("<")






run_tests()
