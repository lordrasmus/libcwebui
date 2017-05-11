

all:
	make -C lib
	make -C LinuxMain
	make -C testSite

fuzzer:
	make -C oss_fuzz

clean:
	make -C lib clean
	make -C LinuxMain clean
	make -C SingleMain clean
	make -C testSite clean
	make -C Docker clean
	
