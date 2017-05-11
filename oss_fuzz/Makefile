
FLAGS=-I../lib/ -I../lib/include -Wswitch-enum

all: ../testSite/src/test_plugin.so
	$(CC) $(FLAGS) -c main.c  -O0 -ggdb3
	$(CC) main.o -O0 -ggdb3 -o main -lWebserver -lssl -levent -levent_pthreads  -L../lib/lib/ -Wl,-rpath=.

#../testSite/www_data__.c	

#	$(CXX) main.o -O0 -ggdb3 -o main -lstdc++ -lWebserver_ltog -lssl -levent -levent_pthreads -L../lib/lib/ -Wl,-rpath=.
#	$(CXX) main.o -O0 -ggdb3 -o main -lstdc++ -lWebserver_gold -lssl -levent -levent_pthreads -L../lib/lib/ -Wl,-rpath=.
	cp ../lib/lib/libWebserver*.so .

../lib/lib/libWebserver.so:
	${MAKE} -C ../lib

../testSite/src/test_plugin.so: ../lib/lib/libWebserver.so
	${MAKE} -C ../testSite/src/
	
clean:
	rm -f main.o main
