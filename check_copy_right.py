#!/usr/bin/python3


import os

from pprint import pprint

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

tmp = get_filepaths("lib")
for f in tmp:
	if "third_party" in f: continue
	
	if f.endswith(".c"):
		text = open(f,"r").read(len(header))
		if not text == header:
			print("update licence : " + f)
			replace_licence( f )
			exit(1)

