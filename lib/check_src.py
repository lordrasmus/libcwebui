#!/usr/bin/python3

import sys


from pprint import pprint

with open( sys.argv[1], "U" ) as f:
	lines = f.readlines()
	f.seek(0)
	orig = f.read()

	if repr(f.newlines) == "'\\n'":
		sys.exit(0)

	pprint ( repr(f.newlines) )


new = ""
for line in lines:
	new += line

if not new == orig:
	print ( new )

with open( sys.argv[1], "w" ) as f:
	f.write( new )


sys.exit(1)
