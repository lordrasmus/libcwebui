import libcwebui

def func1(  ):
    libcwebui.send( "func1 called" )
    libcwebui.setRenderVar( "render1" , "value2")

libcwebui.set_plugin_name("PyPlugin")
libcwebui.register_function( func1 )
