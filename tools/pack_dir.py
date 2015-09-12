#!/usr/bin/python3

import hashlib

import os, struct, gzip

from pprint import pprint

def list_files( startpath, alias ):
	
	file_list = []
	
	st_mtime = 0
	
	for root, dirs, files in os.walk(startpath):
    
		root = root.replace(startpath, '') 
    
		if root.endswith( "/" ):
			root = root[:-1]
			
		for tmp in files:
			
			f = root + "/" + tmp
			
			stat = os.stat( startpath + f )
			if st_mtime < stat.st_mtime:
				st_mtime = stat.st_mtime
				
			file_list.append( {"PATH" : f , "SIZE" : stat.st_size } )
	
	return [ int( st_mtime ), file_list ]

def write_uint32( f, value ):
	#print( int ( value ) )
	data = struct.pack( 'I', int ( value ) )
	for a in data:
		f.write( "" + str( a ) + "," )

def write_uint64( f, value ):
	#print( int ( value ) )
	data = struct.pack( 'L', int ( value ) )
	for a in data:
		f.write( "" + str( a ) + "," )		

def write_string( f, value ):
	
	write_uint32( f, len( value ) )
	
	for a in value:
		f.write( "'" + str( a ) + "'," )
	
	f.write( "'\\0'," )

def bytes_from_file(filename):
    with open(filename, "rb") as f:
        while True:
            byte = f.read(1)
            if not byte:
                break
            yield(ord(byte))



def write_uncompressed_file( f, path , f_data ):
	
	f2 = open( path + f_data["PATH"], "rb")
	
	h = hashlib.md5()
	h.update( f2.read() )
	etag = h.hexdigest().upper()
	f2.close()
		
	write_uint32( f, 0 ) # nicht komprimiert
	write_string( f, f_data["PATH"] )
	write_string( f, etag )
	write_uint32( f, f_data["SIZE"] )
	
	for b in bytes_from_file( path + f_data["PATH"]):
		#pprint(b)
		f.write( "" + str( b ) + "," )
	f.write( "'\\0'," )
	
def write_compressed_file( f, path, f_data ):
				
	with open( path + f_data["PATH"], 'rb') as f_in:
		s_in = f_in.read()
		

		h = hashlib.md5()
		h.update( s_in )
		etag = h.hexdigest().upper()
		
		s_out = gzip.compress(s_in)
		
		if len (s_out) < f_data["SIZE"]:
			
			write_uint32( f, 1 )  # komprimiert
			write_string( f, f_data["PATH"] )
			write_string( f, etag )
			write_uint32( f, f_data["SIZE"] )
			
			write_uint32( f, len (s_out))
			for b in s_out:
				#pprint(b)
				f.write( "" + str( b ) + "," )	
				
		else:
			write_uncompressed_file( f, path, f_data )
			
def gen_c_file( path, alias ):
	
	liste = list_files ( path, alias )
	
	info = { "ALIAS" : alias , "PATH": path , "TIME" : liste[0], "FILES" : liste[1] }
	
	
	#pprint( info )
	
	tmp = path
	if path.endswith("/"):
		tmp = path[:-1]
		
	f_name = tmp + "_data_" + alias.replace("/","_") + ".c"
	
	print("\nwriting : " + f_name )
	print("  Alias : " + info["ALIAS"] )
	print("  Time  : " + str( int( info["TIME"] ) ))
	print("  Files : " + str (len (info["FILES"] ) ) )
	
	with open(  f_name, "w" ) as f:
	
		f.write("\n")
		
		f.write("char data_" + alias.replace("/","_") + "[] = {" )
		
		write_string( f, info["ALIAS"] )
		write_uint64( f, info["TIME"] )
		#write_uint64( f, 1 )
		
		write_uint32( f, len( info["FILES"] ) )
		for f_data in info["FILES"]:
			
			#write_uncompressed_file( f, info["PATH"],  f_data )
			write_compressed_file( f, info["PATH"],  f_data )
			
			
			
		f.write("0};\n")
		f.write("\n")
	
	st = os.stat(f_name)
	print("  c code size   : " + str( int ( st.st_size / 1024 ) ) + " kB" )
	
	os.system("gcc -c " + f_name + " -o /tmp/comp_test ")
	st = os.stat("/tmp/comp_test")
	print("  c object size : " + str( int ( st.st_size / 1024 ) ) + " kB" )
	
	
       
gen_c_file( "../testSite/www/" , "/" )

#gen_c_file( "." , "/tools" )
