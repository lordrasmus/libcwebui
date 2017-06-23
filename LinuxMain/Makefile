
FLAGS=-I../lib/ -I../lib/include -Wswitch-enum

all: ../testSite/src/test_plugin.so
	$(CXX) $(FLAGS) -c main.cpp  -O0 -ggdb3 -fsanitize=address
	$(CXX) main.o -O0 -ggdb3 -o main -lstdc++ -lWebserver -lssl -levent -levent_pthreads  -L../lib/lib/ -Wl,-rpath=. -fsanitize=address

	cp ../lib/lib/libWebserver*.so .

../lib/lib/libWebserver.so:
	${MAKE} -C ../lib

../testSite/src/test_plugin.so: ../lib/lib/libWebserver.so
	${MAKE} -C ../testSite/src/
	
clean:
	rm -f main.o main
