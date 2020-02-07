

all:
	$(MAKE) -C lib
	$(MAKE) -C Samples/Linux/SharedMain
	$(MAKE) -C Samples/testSite
	
	@echo ""
	@echo "run -> make run"
	@echo ""

run: all
	( cd Samples/Linux/SharedMain/; ./main )

fuzzer:
	$(MAKE) -C oss_fuzz

clean:
	$(MAKE) -C lib clean
	$(MAKE) -C Samples/Linux/SharedMain clean
	$(MAKE) -C Samples/Linux/SingleMain clean
	$(MAKE) -C Samples/Linux/SingleMainPython clean
	$(MAKE) -C Samples/testSite clean
	$(MAKE) -C Docker clean

unittests:
	$(MAKE) -C tests

linux_tests:
	$(MAKE) -C lib
	$(MAKE) -C Samples/testSite
	$(MAKE) -C Samples/Linux/SharedMain
	$(MAKE) -C Samples/Linux/SingleMain
	$(MAKE) -C Samples/Linux/SingleMainPython
	$(MAKE) -C tests/config_variants/LinuxWebsocketSelect
	$(MAKE) -C tests/config_variants/LinuxWebsocketNoSSL
	$(MAKE) -C tests/config_variants/LinuxSelect
#	$(MAKE) -C tests/config_variants/LinuxEpoll

osx_tests:
	$(MAKE) -C Samples/OSX/MakeProject
	$(MAKE) -C tests/config_variants/LinuxWebsocketSelect
	$(MAKE) -C tests/config_variants/LinuxWebsocketNoSSL
	
