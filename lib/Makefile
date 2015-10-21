SHELL=/bin/bash

DEFS= -DLINUX

#
#	WICHTIG muss beim compilieren UND linken gleich sein
#
COMP_OPT= -O0 -ggdb3

PY_FLAGS = -I/usr/include/python3.4
PY_LINK = -lpython3.4

PY_FLAGS = -I/usr/include/python2.7
PY_LINK = -lpython2.7

FLAGS= $(COMP_OPT) -Wall -fPIC -Wextra   -std=c99  -D_POSIX_SOURCE -Wno-unused-parameter -Werror=implicit-function-declaration \
	-fdata-sections -ffunction-sections -fvisibility=hidden  -Wno-variadic-macros -Wpointer-arith \
	$(SIZE_OPTS) $(PY_FLAGS)

#-pedantic  -save-temps

SIZE_OPTS= -finline -fno-inline-functions -finline-functions-called-once \
	--param max-inline-insns-auto=62 --param inline-unit-growth=0 --param large-unit-insns=0 #--param inline-call-cost=4

#	-finline-functions -fno-inline-functions-called-once

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
	@mkdir -p obj/python
	@mkdir -p obj/platform
	@mkdir -p obj/platform/linux
	@mkdir -p obj/third_party/src/simclist/
	@mkdir -p obj/third_party/src/rb_tree/
	@mkdir -p obj/third_party/src/miniz/

	( if [ ! -e WebserverConfig.h ] ; then cp WebserverConfig.h.sample WebserverConfig.h ; fi )

	@$(MAKE) build

OBJ_LIST = obj/src/*.o obj/platform/linux/*.o obj/ssl/*.o obj/python/*.o obj/third_party/src/rb_tree/*.o obj/third_party/src/simclist/*.o obj/third_party/src/miniz/*.o
OBJ_LIST_LTO = obj/src/*.o.lto obj/platform/linux/*.o.lto obj/ssl/*.o.lto obj/third_party/src/rb_tree/*.o.lto obj/third_party/src/simclist/*.o.lto

LINK_OPTS =  -shared -Wl,-O2 -Wl,--gc-sections $(PY_LINK) #-Wl,--strip-all

build: $(OBJ)
	@echo 'Building Lib : libWebserver.so'

# 	ld / gold no lto link
#	@echo -e "\033[01;32mnormal link\033[00m"
	$(COMPILER)  $(LINK_OPTS) -Wl,-soname,libWebserver.so  -o lib/libWebserver.so $(OBJ_LIST)
#	$(COMPILER) -fuse-ld=gold -Wl,--icf=all  $(LINK_OPTS) -Wl,-soname,libWebserver_gold.so  -o lib/libWebserver_gold.so $(OBJ_LIST)

# 	ld / gold lto link
#	@echo -e "\033[01;32mlto link\033[00m"
#	$(COMPILER) -Os -flto  $(LINK_OPTS) -Wl,-soname,libWebserver_lto.so -o lib/libWebserver_lto.so $(OBJ_LIST_LTO)
#	$(COMPILER) -Os -flto -fuse-ld=gold -Wl,--icf=all   $(LINK_OPTS) -Wl,-soname,libWebserver_ltog.so -o lib/libWebserver_ltog.so $(OBJ_LIST_LTO)


	@echo -e "\033[01;32mexported symbols : "$$(nm -C lib/libWebserver.so | grep " T " | wc -l)"\033[00m"


obj/%.o : %.c
	@echo 'Compiling File : $<'
#	./check_src.py $<
	@$(COMPILER) $(INCLUDES) $(DEFS) $(FLAGS) -fno-lto -c -o $@ $<
#	@$(COMPILER) $(INCLUDES) $(DEFS) $(FLAGS) -flto -c -o $@.lto $<


clean:
	@echo 'Deleting Object Files'

	@rm -rvf .lib/obj/*
	@rm -rvf obj/*



#gold --icf all  --hash-style=gnu -soname,libWebserver_lto.so -o lib/libWebserver_lto.so $(OBJ_LIST) \
	#-lpthread -lssl -lc -ldl -lcrypto -levent   \
	#/usr/lib/x86_64-linux-gnu/libc_nonshared.a \
	#/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/4.8/crtbeginS.o \
	#--no-as-needed /usr/lib/gcc/x86_64-linux-gnu/4.8/crtendS.o \
	#/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../x86_64-linux-gnu/crtn.o

	#gold --icf all -lpthread -lssl -lc -lgcc -ldl -lcrypto -levent -soname,libWebserver_lto.so -o lib/libWebserver_lto.so $(OBJ_LIST)


