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


def run_tests( base_dir ):

	parser = argparse.ArgumentParser(description="Execute Header Parser Tests")
	parser.add_argument('--verbose', help='Verbose Error Reporting', action='store_true')
	args = parser.parse_args()

	os.system("make -j 2")
	os.system("mkdir -p results")

	ret = subprocess.getstatusoutput( "find ../../fuzzing/" + base_dir + "/input/* | grep -v README.txt")
	liste = ret[1].split("\n")

	test_failed=False

	#pprint( liste )

	for test in liste:

		sys.stderr.write("run test : " + test + " \x1b[60G -> ")
		sys.stderr.flush()

		ret = subprocess.getstatusoutput("./main.gcc < " + test + " > /tmp/test_out" )
		if not ret[0] == 0:
			print("Error: Parser Fehler\n" + ret[1] )
			continue

		result_file = "results/" + os.path.basename( test )

		if not os.path.exists( result_file ):
			print("Error: keine Result Datei -> %s\n" % result_file)
			print("\x1b[32m<---------- Input ----------   " + test + " >\x1b[0m")
			os.system("cat " + test )
			print("\x1b[32m<---------- Output ---------   /tmp/test_out  >\x1b[0m")
			os.system("cat /tmp/test_out" )
			print("\x1b[32m<---------- Soll  ----------   " + result_file + " >\x1b[0m")

			ans = question_yes_no("Result File erzeugen ?","j")
			if ans == "True":
				#f = open( result_file, "w")
				#f.write( ret[1] )
				#f.close()
				os.system("cp /tmp/test_out " + result_file )
				print("\nResult file erzeugt\n")

			continue


		ret = os.system("diff -u /tmp/test_out " + result_file + " > /dev/null" )
		if ret == 0:
			print("\x1b[32mpass\x1b[0m")
		else:
			print("\x1b[31mfail\x1b[0m")

			test_failed = True

			if args.verbose is True:
				print("\x1b[32m<---------- Input ----------   " + test + " >\x1b[0m")
				os.system("cat " + test )
				print("\x1b[32m< Output /tmp/test_out  >\x1b[0m")
				os.system("cat /tmp/test_out" )
				print("\x1b[32m< Soll   " +  result_file + " >\x1b[0m")
				os.system("cat " + result_file )
				print("\x1b[32m< Diff diff -u " + result_file + " /tmp/test_out >\x1b[0m")
				os.system("diff -u " + result_file + " /tmp/test_out" )
				print("\x1b[32m< Ende >\x1b[0m")
				
				value = input("w = weiter, u = update : ")
				if value == "w":
					continue
				
				if value == "u":
					os.system("cp /tmp/test_out " + result_file )
					continue
				
				exit(1)


	if test_failed:
		return 1
	
	return 0
