

all:
	make -C lib
	make -C Samples/Linux/SharedMain
	make -C testSite

fuzzer:
	make -C oss_fuzz

clean:
	make -C lib clean
	make -C Samples/Linux/SharedMain clean
	make -C SingleMain clean
	make -C testSite clean
	make -C Docker clean
	
