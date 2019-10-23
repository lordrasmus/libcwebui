
CFLAGS=-fPIC -I../../../lib/include/ -I../../../lib/include/intern/ -I../../../lib/ 

OBJ=testfunktions.o  var_test.o  webserver_api.o  websocket_api.o


all: $(OBJ)
	$(CC) -Wall -ggdb3 -fPIC  -shared  $(OBJ) -o test_plugin.so   ../../../lib/lib/libWebserver.so

clean:
	rm -f $(OBJ) test_plugin.so
