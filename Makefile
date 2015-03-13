

all:
	make -C lib
	make -C LinuxMain
	make -C testSite
	
clean:
	make -C lib clean
	make -C LinuxMain clean
	make -C testSite clean
