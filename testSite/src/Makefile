
INCLUDES=-I../../lib/third_party/src/simclist/ -I../../lib/include/ -I../../lib/

all:
	$(CC) $(INCLUDES) -E  websocket_api.c -o  websocket_api.c.list
	$(CC) $(INCLUDES) -E  webserver_api_functions.c -o  webserver_api_functions.c.list
	$(CC) -Wall -g -fvisibility=hidden $(INCLUDES) -fPIC  -shared -Wl,-soname,test_plugin.so -o test_plugin.so  testfunktions.c  var_test.c  webserver_api_functions.c  websocket_api.c ../../lib/lib/libWebserver.so
