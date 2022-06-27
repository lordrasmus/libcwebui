
FLAGS=-I../../../lib -I../../../lib/include -Wswitch-enum


#FLAGS += -fsanitize=address
#FLAGS += -fsanitize=thread
#FLAGS += -fsanitize=undefined

PY_FLAGS = $(shell python-config --includes)
PY_LINK = $(shell python-config --libs --embed)

all: ../../testSite/src/test_plugin.so
	
	echo $(PY_LINK)
	
	cp ../../../lib/lib/libWebserver.so .
	
	$(CXX) $(FLAGS) -c main.cpp  -O0 -ggdb3 
	$(CXX) $(FLAGS) main.o -O0 -ggdb3 -o main -lstdc++ -lWebserver -lssl -levent -levent_pthreads $(PY_LINK)  -L../../../lib/lib/ -Wl,-rpath=.
	

../../../lib/lib/libWebserver.so:
	${MAKE} -C ../../../lib

../../testSite/src/test_plugin.so: ../../../lib/lib/libWebserver.so
	${MAKE} -C ../../testSite/src/
	
clean:
	rm -f main.o main
	${MAKE} -C ../../../lib clean

