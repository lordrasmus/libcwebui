# libcwebui

compact webserver library for WebUIs

##Features:
<ul>
<li>    Small ( usable in Embedded Systems )
<li>	Fast
<li>	SSL
<li>	Websocket Support
<li>	Template Engine
<li>    Python Plugin Support
<li>    Portable
</ul>


## Sample

### Files

 *   main.c
 *   plugin.py
 *   www/index.html
 *   img/img.jpg

### main.c

```
int main(int argc, char **argv) {
	
	   if (0 == WebserverInit()) {

       		WebserverConfigSetInt("port",8080);
    		WebserverAddFileDir("", "www");
    		WebserverAddFileDir("img", "img");

            WebserverInitPython();
            WebserverLoadPyPlugin( "plugin.py" );

    		WebserverStart();
    	}
    	return 0;
}

```

### plugin.py

```
import libcwebui

def func1(  ):
    libcwebui.send( "  render b : " )
    libcwebui.setRenderVar( "render1" )

libcwebui.set_plugin_name("PyPlugin")
libcwebui.register_function( func1 )
```

### www/index.html

```
TEMPLATE_V1
Call Function   : {f:func1}
Render Variable : {get:render;"render1"}
```