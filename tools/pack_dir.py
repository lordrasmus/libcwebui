#!/usr/bin/python3

import hashlib

import os, struct, gzip, zlib

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


def check_template( f,  path , f_data ):
	
	f2 = open( path + f_data["PATH"], "rb")
	data = f2.read(20)
	f2.close()
	
	#print("check template: " + f_data["PATH"] )
	
	
	template = 0
	if data.startswith(b"TEMPLATE_V1"):
		template=1
		#print(" template")
		
	if f_data["PATH"].endswith(".inc"):
		template=1
		#print(" template")
	
	write_uint32( f, template )
	
	return template
	

def write_uncompressed_file( f, path , f_data ):
	
	f2 = open( path + f_data["PATH"], "rb")
	data = f2.read()
	f2.close()
	
	h = hashlib.md5()
	h.update( data )
	etag = h.hexdigest().upper()
	
		
	write_uint32( f, 0 ) # nicht komprimiert
	write_string( f, f_data["PATH"] )
	write_string( f, etag )
	tp = check_template( f, path, f_data )	# auf template prüfen
	write_uint32( f, f_data["SIZE"] )
	
	add = ""
	if tp == 1:
		add = "  ( template ) "
	print("not compressing        : " + f_data["PATH"] + add )
	
	
	for b in bytes_from_file( path + f_data["PATH"]):
		#pprint(b)
		f.write( "" + str( b ) + "," )
	f.write( "'\\0'," )

def compress_data( data ):
	
	
	#f2 = open( "dict" , "rb")
	#dictator = f2.read()
	#f2.close()
	
	#dictator= None
	
	strats = [ 
		[ zlib.Z_DEFAULT_STRATEGY, 0 , None],
		[ zlib.Z_FILTERED, 0 , None],
		[ zlib.Z_HUFFMAN_ONLY, 0, None ],
#		[ zlib.Z_DEFAULT_STRATEGY, 0 , dictator],
#		[ zlib.Z_FILTERED, 0 , dictator],
#		[ zlib.Z_HUFFMAN_ONLY, 0, dictator ]
	]
	outs = []
	
	for strat in strats:
		if strat[2] == None:
			compress = zlib.compressobj( level=9, method=zlib.DEFLATED, wbits=-15 ,memLevel=9, strategy=strat[0] )
		else:
			compress = zlib.compressobj( level=9, method=zlib.DEFLATED, wbits=-15 ,memLevel=9, strategy=strat[0] , zdict=strat[2] )
		deflated = compress.compress(data)
		deflated += compress.flush()
		outs.append( [ strat[0], deflated ] )
		strat[1] = len( deflated )
	
	ret = [ 0, len( data ) , None]
	for strat in strats:
		if ret[1] > strat[1]:
			ret = strat
	
	#for a in strats:
	#	print( str( a[0] ) + " : " + str( a[1] ) )
		
	#pprint( ret )
	if ret[0] == zlib.Z_DEFAULT_STRATEGY:
		t_out = "zlib.Z_DEFAULT "
		
	if ret[0] == zlib.Z_FILTERED:
		t_out = "zlib.Z_FILTERED"
		
	if ret[0] == zlib.Z_HUFFMAN_ONLY:
		t_out = "zlib.Z_HUFFMAN "
		
	if not ret[2] == None:
		t_out += " ( dict ) "
	
	s_out = None
	
	for out in outs:
		if out[0] == ret[0]:
			s_out = out[1]
	
	return [ s_out, t_out ]
	

def write_compressed_file( f, path, f_data ):
				
	with open( path + f_data["PATH"], 'rb') as f_in:
		s_in = f_in.read()
		
		# etag generieren
		h = hashlib.md5()
		h.update( s_in )
		etag = h.hexdigest().upper()
		
		
		ret = compress_data( s_in )
		s_out = ret[0]
		t_out = ret[1]
		
		#s_out = gzip.compress(s_in)
		#print( "l gzip : " + str( len( s_out ) ) )
		
		if len (s_out) < f_data["SIZE"]:
			
			
			
			write_uint32( f, 2 )  # komprimiert
			write_string( f, f_data["PATH"] )
			write_string( f, etag )
			tp = check_template( f, path, f_data )	# auf template prüfen
			write_uint32( f, f_data["SIZE"] )
			
			text = "compressing ( " 
			#text += t_out + " "
			#text += str( f_data["SIZE"]) + " / " + str( len (s_out) ) + " "
			text += str ( int ( ( len (s_out) / f_data["SIZE"] ) * 100 ) ) + " % " 
			text += " )  : " + f_data["PATH"] 
			if  tp == 1 :
				text += "  ( template )"
			print( text )
			
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
		
		f.write("unsigned char data_" + alias.replace("/","_") + "[] = {" )
		
		write_string( f, info["ALIAS"] )
		write_uint64( f, info["TIME"] )
		#write_uint64( f, 1 )
		
		write_uint32( f, len( info["FILES"] ) )
		for f_data in info["FILES"]:
			#pprint( f_data )
			if f_data["PATH"].lower().endswith(".jpg"): 
				write_uncompressed_file( f, info["PATH"],  f_data )
				continue
			write_compressed_file( f, info["PATH"],  f_data )
			
			
			
		f.write("0};\n")
		f.write("\n")
	
	st = os.stat(f_name)
	print("  c code size   : " + str( int ( st.st_size / 1024 ) ) + " kB" )
	
	os.system("gcc -O0 -c " + f_name + " -o /tmp/comp_test ")
	st = os.stat("/tmp/comp_test")
	print("  c object size : " + str( int ( st.st_size / 1024 ) ) + " kB" )
	
	
       
gen_c_file( "../testSite/www/" , "/" )
#gen_c_file( "../testSite/img/" , "img" )

#gen_c_file( "." , "/tools" )
