# libcwebui

[![Build Status](https://travis-ci.org/lordrasmus/libcwebui.svg?branch=master)](https://travis-ci.org/lordrasmus/libcwebui)
[![Scan Status](https://scan.coverity.com/projects/7148/badge.svg?flat=1)](https://scan.coverity.com/projects/lordrasmus-libcwebui)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/3c4b64df7ceb4bc895a0da5a63f21a9e)](https://www.codacy.com/app/ramin/libcwebui?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=lordrasmus/libcwebui&amp;utm_campaign=Badge_Grade)
![license](https://img.shields.io/badge/license-MPL2-orange.svg)

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
