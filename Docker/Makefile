

FLAGS=-I../lib/ -I../lib/include

	
all: main
	cp -u ../lib/lib/libWebserver.so .
	cp -ru ../testSite .
	cp -u ../LinuxMain/*.pem .
	sudo docker build -t lordrasmus/libcwebui .
	sudo docker run --rm=true -p 8080:8080 -p 4443:4443 --net=host lordrasmus/libcwebui


../lib/lib/libWebserver.so:
	${MAKE} -C ../lib

main: main.cpp ../lib/lib/libWebserver.so
	$(CXX) $(FLAGS) main.cpp -O3 -o main -lstdc++ -lWebserver -lssl -levent -levent_pthreads  -L../lib/lib/ -Wl,-rpath=.


clean:
	${MAKE} -C ../lib clean
	rm -f main libWebserver.so ../lib/lib/libWebserver.so
