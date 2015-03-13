#!/usr/bin/python3


import os


header ="""/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/
"""

def get_filepaths(directory):
    """
    This function will generate the file names in a directory 
    tree by walking the tree either top-down or bottom-up. For each 
    directory in the tree rooted at directory top (including top itself), 
    it yields a 3-tuple (dirpath, dirnames, filenames).
    """
    file_paths = []  # List which will store all of the full filepaths.

    # Walk the tree.
    for root, directories, files in os.walk(directory):
        for filename in files:
            # Join the two strings in order to form the full filepath.
            filepath = os.path.join(root, filename)
            file_paths.append(filepath)  # Add it to the list.

    return file_paths  # Self-explanatory.

def replace_licence( path ):
	f = open( path ,"r")
	text = f.read()
	f.close()
	
	t2 = text.split("*/",1)
	t2[0] = header
	
	text = "".join( t2 )
	
	f = open( path ,"w")
	f.write( text )
	f.close()
	

#print ( header)

#
#	check copyright / license
#
tmp = get_filepaths("lib")
for f in tmp:
	if "third_party" in f: continue
	if "JSON_parser.c" in f: continue
	
	if f.endswith(".c") or f.endswith(".h"):
		text = open(f,"r").read(len(header))
		if not text == header:
			print("\nupdate licence : " + f + "\n")
			replace_licence( f )
			os.system("git diff " + f )
			print('git add starten ?')
			x = input()
			if x == "j":
				os.system("git add " + f )
			else:
				os.system("git checkout " + f )
				os.system("geany " + f )
				
			exit(1)
			
	
#
#  check lin endings
#
for path in tmp:
	if path.endswith(".c") or path.endswith(".h"):
		f = open( path ,"r")
		text = f.read()
		f.close()
		
		f = open( path + ".tmp" ,"w")
		f.write( text )
		f.close()
		
		ret = os.system("diff " + path + ".tmp " + path + " > /dev/null")
		
		if ret == 0:
			os.system("rm " + path + ".tmp" )
			continue
			
		print("\n  line ending wrong : " + path + " \n")
		#os.system("geany " + path + ".tmp" )
		
		print('rewrite und git add starten ?')
		x = input()
		if x == "j":
			os.system("mv " + path + ".tmp " + path  )
			os.system("git add " + path )
		else:
			os.system("rm " + path + ".tmp" )
		
		exit(1)
