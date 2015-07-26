"""Python Test Module"""


import libcwebui

from pprint import pprint


def test( a ):
	"""JO"""
	global b
	#from pprint import pprint
	#pprint ( a )
	b = b * 2
	print("Test out : " + str( a ) )
	print("Global : " +  str( b ) )
	g = "g"
	#pprint( dir () )
	#pprint( dir (keyword) )

def test2( a ):
	print("JO test2")

print("")
print("  Python Test Script " )
print("")

b = 5
c=6
d="a"

print( b )
print( c )
print( d )

libcwebui.sayHello()

libcwebui.set_plugin_name("Test1")

def cc():

	libcwebui.register_function( test )
	libcwebui.register_function( test2 )


def jj():
	cc()

jj()

def ee():
	gg

def hh():
	ee()

#hh()
