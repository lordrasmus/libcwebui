

FLAGS=-I../lib/ -I../lib/include

	
all: main
	cp -u ../lib/lib/libWebserver.so .
	cp -ru ../testSite .
	cp -u ../LinuxMain/*.pem .
	rm -f testSite/*.c
	sudo docker build -t lordrasmus/libcwebui .

run: all
	sudo docker run --rm=true -p 8080:8080 -p 4443:4443 -p 4444:4444 --net=bridge lordrasmus/libcwebui


../lib/lib/libWebserver.so:
	${MAKE} -C ../lib

../testSite/src/test_plugin.so: ../lib/lib/libWebserver.so
	${MAKE} -C ../testSite/src/

main: main.cpp ../testSite/src/test_plugin.so
	$(CXX) $(FLAGS) main.cpp -O3 -o main -lstdc++ -lWebserver -lssl -levent -levent_pthreads  -L../lib/lib/ -Wl,-rpath=.


clean:
	${MAKE} -C ../lib clean
	${MAKE} -C ../Samples/testSite clean
	rm -f main libWebserver.so ../lib/lib/libWebserver.so
