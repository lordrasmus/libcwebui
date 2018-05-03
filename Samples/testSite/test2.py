"""Python Test Module2"""
from pprint import pprint


import libcwebui

#pprint( dir () )

print("")
print("  Python Test Script 2 " )
print("")



def test3(  ):
	"""JO"""
	global b
	#print("JO test3")
	libcwebui.send("b : " + str( b ))

def test4(  ):
	"""JO"""
	global b
	#print("JO test4")
	libcwebui.send("b : ")
	libcwebui.send( b )
	libcwebui.send( "<br>" )


	libcwebui.send( "  render b : " )
	ret = libcwebui.getRenderVar( "b" )
	libcwebui.send( ret )
	libcwebui.send( "<br>" )

	libcwebui.send( "  session b : " )
	ret = libcwebui.getSessionVar( "STORE_NORMAL", "b" )
	libcwebui.send( ret )
	libcwebui.send( "<br>" )

	ret = libcwebui.getURLParameter( "a" )
	if ret == None:
		ret = "None"
	libcwebui.send( "Param a : " + ret )


libcwebui.set_plugin_name("PyTest2")


#libcwebui.register_function( test )
libcwebui.register_function( test3 )
libcwebui.register_function( test4 )

b = "hh"

print( "b : " + str( b ) )
