SHELL=/bin/bash

DEFS= -DLINUX

FLAGS= -Wall -fPIC -Wextra -O0 -ggdb3 -std=c99  -D_POSIX_SOURCE -Wno-unused-parameter \
	-fdata-sections -ffunction-sections -fvisibility=hidden 

#-fvisibility-inlines-hidden
#FLAGS+= -Wpedantic

INCLUDES= -I. -Iinclude -Iplatform/include -Ithird_party/src/simclist/ -Ithird_party/src/rb_tree/
LIBS= 

COMPILER=$(CC)

include Makefile.files

OBJ_T := $(patsubst %.c,%.o,$(SOURCES))
OBJ := $(patsubst %,obj/%,$(OBJ_T))

all:
	@echo '------------------------------------------------'
	@echo 'Includes : ' $(INCLUDES)
	@echo '------------------------------------------------'
	@echo 'Compiler : ' $(COMPILER)
	@echo '------------------------------------------------'
	@echo 'Defs     : ' $(DEFS)
	@echo '------------------------------------------------'
	@echo 'Flags     : ' $(FLAGS)
	@echo '------------------------------------------------'

	@mkdir -p lib

	@mkdir -p obj
	@mkdir -p obj/src
	@mkdir -p obj/ssl
	@mkdir -p obj/platform
	@mkdir -p obj/platform/linux
	@mkdir -p obj/third_party/src/simclist/
	@mkdir -p obj/third_party/src/rb_tree/

	( if [ ! -e WebserverConfig.h ] ; then cp WebserverConfig.h.sample WebserverConfig.h ; fi )
	
	@$(MAKE) build


build: $(OBJ)
	@echo 'Building Lib : libWebserver.so'	
	$(COMPILER) -shared   -Wl,-soname,libWebserver.so -Wl,--gc-sections -o lib/libWebserver.so \
		obj/src/*.o obj/platform/linux/*.o obj/ssl/*.o obj/third_party/src/rb_tree/*.o \
		obj/third_party/src/simclist/*.o 
	@echo -e "\033[01;32mexported symbols : "$$(nm -C lib/libWebserver.so | grep " T " | wc -l)"\033[00m"
	

obj/%.o : %.c
	@echo 'Compiling File : $<'
	@$(COMPILER) $(INCLUDES) $(DEFS) $(FLAGS) -c -o $@ $<


clean:
	@echo 'Deleting Object Files'
	
	@rm -rvf .lib/obj/*
	@rm -rvf obj/*




