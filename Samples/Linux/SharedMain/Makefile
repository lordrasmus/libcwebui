
FLAGS=-I../../../lib -I../../../lib/include -Wswitch-enum


#FLAGS += -fsanitize=address
#FLAGS += -fsanitize=thread
#FLAGS += -fsanitize=undefined

PYTHON_PKG := $(shell pkg-config --modversion python3 )

PY_FLAGS = $(shell pkg-config --cflags python-$(PYTHON_PKG)-embed)
PY_LINK  = $(shell pkg-config --libs   python-$(PYTHON_PKG)-embed)

#
# bei neueren python-config versionen muss man wohl noch  --embed anhängen
# bei älteren geht das aber nicht
#

all: ../../testSite/src/test_plugin.so
	
	echo $(PY_LINK)
	
	cp ../../../lib/lib/libWebserver.so .
	
	$(CXX) $(FLAGS) $(PY_FLAGS) -c main.cpp  -O0 -ggdb3 
	$(CXX) $(FLAGS) main.o -O0 -ggdb3 -o main -lstdc++ -lWebserver -lssl -levent -levent_pthreads $(PY_LINK)  -L../../../lib/lib/ -Wl,-rpath=.
	

../../../lib/lib/libWebserver.so:
	${MAKE} -C ../../../lib

../../testSite/src/test_plugin.so: ../../../lib/lib/libWebserver.so
	${MAKE} -C ../../testSite/src/
	
clean:
	rm -f main.o main
	${MAKE} -C ../../../lib clean

