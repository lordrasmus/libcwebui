
FLAGS=-I../lib/ -I../lib/include

LIB_FLAGS=-fPIC -I../lib/third_party/src/simclist/ -I../lib/include/ -I../lib/

all:
	cp ../lib/lib/libWebserver.so .
#	gcc ${LIB_FLAGS} -fvisibility=hidden -c  plugin.c -o plugin.o
#	gcc ${LIB_FLAGS} -fvisibility=hidden -shared -Wl,-soname,plugin.so plugin.o -o plugin.so -lWebserver -L. -Wl,-rpath=.
	gcc ${FLAGS} main.c plugin.c -o main -lWebserver -lpython2.7 -lssl -levent -levent_pthreads -L. -Wl,-rpath=.

clean:
	rm -f main
