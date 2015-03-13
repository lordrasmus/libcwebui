"""Python Test Module"""


import sys

import libcwebui, traceback, inspect

from pprint import pprint


def test1(  ):
	"""JO"""
	global b
	#print("JO test1")
	libcwebui.send("b : " + str( b ))



def test2(  ):
	global b
	#print("JO test2")
	#pprint( b )
	b += 1
	libcwebui.send("b : " + str( b ))

	try:
		libcwebui.setRenderVar( "a", 2 )

		libcwebui.setRenderVar( "b", b )

		libcwebui.setRenderVar( "c","c" )

		ret = libcwebui.getSessionVar( "STORE_NORMAL", "b" )
		if ret == None:
			ret = 1
		ret += 1
		libcwebui.setSessionVar( "STORE_NORMAL", "b",ret )

	except:
		traceback.print_exc(file=sys.stdout)

def py_version(  ):
	libcwebui.send("Python version: {}.{}.{}".format( sys.version_info.major, sys.version_info.minor, sys.version_info.micro) )
	
	
	
print("")
print("  Python Test Script " )
print("Python version: {}.{}.{}".format( sys.version_info.major, sys.version_info.minor, sys.version_info.micro) )
print("")

#pprint ( dir ( test1 ) )

libcwebui.set_plugin_name("PyTest1")
libcwebui.register_function( test1 )
libcwebui.register_function( test2 )
libcwebui.register_function( py_version )

b = 5

def set_test( a ):
	pprint ( a )
	try:
		libcwebui.setRenderVar( str(a),a )
		libcwebui.getRenderVar( str(a) )
	except:
		traceback.print_exc(file=sys.stdout)

#set_test( b )
#set_test( "c" )
#set_test( test2 )
#set_test( ["a","b"] )
#set_test( {"a":"a","b":"b"} )

print( "b : " + str( b ) )























def test2_bak( a ):
	global b
	#print("JO test2")
	b += 1
	libcwebui.send("b : " + str( b ))

	try:
		libcwebui.setRenderVar( "a",a )
		ret = libcwebui.getRenderVar( "a" )
		pprint ( ret )

		libcwebui.setRenderVar( "b",b )
		ret = libcwebui.getRenderVar( "b" )
		pprint ( ret )

		libcwebui.setRenderVar( "c","c" )
		ret = libcwebui.getRenderVar( "c" )
		pprint ( ret )

	except:
		traceback.print_exc(file=sys.stdout)



