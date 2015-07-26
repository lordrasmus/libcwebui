"""Python Test Module2"""
from pprint import pprint


import libcwebui

#pprint( dir () )

print("")
print("  Python Test Script 2 " )
print("")

b = "hh"
c="jj"
d="tz"

print( b )
print( c )
print( d )


def test( a ):
	"""JO"""
	global b
	b = b * 5
	print("Test2 out : " + str( a ) )
	print("Global : " +  str( b ) )
	

libcwebui.set_plugin_name("Test2")

libcwebui.register_function( test )
