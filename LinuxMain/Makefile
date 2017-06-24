
FLAGS=-I../lib/ -I../lib/include -Wswitch-enum


FLAGS += -fsanitize=address
#FLAGS += -fsanitize=thread
#FLAGS += -fsanitize=undefined

all: ../testSite/src/test_plugin.so
	$(CXX) $(FLAGS) -c main.cpp  -O0 -ggdb3 
	$(CXX) $(FLAGS) main.o -O0 -ggdb3 -o main -lstdc++ -lWebserver -lssl -levent -levent_pthreads  -L../lib/lib/ -Wl,-rpath=.

	cp ../lib/lib/libWebserver*.so .

../lib/lib/libWebserver.so:
	${MAKE} -C ../lib

../testSite/src/test_plugin.so: ../lib/lib/libWebserver.so
	${MAKE} -C ../testSite/src/
	
clean:
	rm -f main.o main

