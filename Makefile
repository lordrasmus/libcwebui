

all:
	make -C lib
	make -C Samples/Linux/SharedMain
	make -C Samples/testSite
	
	@echo ""
	@echo "run -> make run"
	@echo ""

run: all
	( cd Samples/Linux/SharedMain/; ./main )

fuzzer:
	make -C oss_fuzz

clean:
	make -C lib clean
	make -C Samples/Linux/SharedMain clean
	make -C Samples/Linux/SingleMain clean
	make -C Samples/Linux/SingleMainPython clean
	make -C Samples/testSite clean
	make -C Docker clean
	
