
FLAGS=-I../lib/ -I../lib/include

all:
	$(CXX) $(FLAGS) -c main.cpp  -O0 -ggdb3
	$(CXX) ../testSite/www_data__.c main.o -O0 -ggdb3 -o main -lstdc++ -lWebserver -lssl -levent -levent_pthreads  -L../lib/lib/ -Wl,-rpath=.
#	$(CXX) main.o -O0 -ggdb3 -o main -lstdc++ -lWebserver_ltog -lssl -levent -levent_pthreads -L../lib/lib/ -Wl,-rpath=.
#	$(CXX) main.o -O0 -ggdb3 -o main -lstdc++ -lWebserver_gold -lssl -levent -levent_pthreads -L../lib/lib/ -Wl,-rpath=.
	cp ../lib/lib/libWebserver*.so .

clean:
	rm -f main.o main
