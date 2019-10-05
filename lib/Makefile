SHELL=/bin/bash

#ifeq ($(TRAVIS_BUILD),true)
#    DEFS= -D${TRAVIS_OS_NAME}
#else
#    DEFS= -DLINUX
#endif

#
#	WICHTIG muss beim compilieren UND linken gleich sein
#
COMP_OPT= -O0 -ggdb3


PY_FLAGS = $(shell python-config --includes)
PY_LINK = $(shell python-config --libs)

#PY_FLAGS = $(shell python3-config --includes)
#PY_LINK = $(shell python3-config --libs)


FLAGS= $(COMP_OPT) -Wall -fPIC -Wextra   -std=c99  -D_POSIX_SOURCE \
	-DMINIZ_NO_ARCHIVE_APIS -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES \
	-fdata-sections -ffunction-sections -fvisibility=hidden -Wpointer-arith \
	$(SIZE_OPTS) $(PY_FLAGS) -DUSE_NEW_LIST \
	-Wno-return-type -Wno-pointer-sign -Wno-variadic-macros  -Wno-unused-parameter \
	-Werror=implicit-function-declaration -Werror=return-type  -Werror=missing-declarations \
	-Werror=old-style-definition -Werror=format -Wformat=2

ifneq ($(TRAVIS_BUILD),true)
#	FLAGS += -fsanitize=address
#	FLAGS += -fsanitize=thread
#	FLAGS += -fsanitize=undefined
endif
	

SIZE_OPTS= -finline -fno-inline-functions

#FLAGS+= -Wpedantic

INCLUDES= -I. -Iinclude -Iplatform/include -Ithird_party/src/rb_tree/ -Ithird_party/src/is_utf8/ -Ithird_party/src/miniz/
LIBS=

include Makefile.files

OBJ_T := $(patsubst %.c,%.o,$(SOURCES))
OBJ := $(patsubst %,obj/%,$(OBJ_T))

all:
	@echo '------------------------------------------------'
	@echo 'Includes : ' $(INCLUDES)
	@echo '------------------------------------------------'
	@echo 'Compiler : ' $(CC)
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
	@mkdir -p obj/third_party/src/rb_tree/
	@mkdir -p obj/third_party/src/miniz/
	@mkdir -p obj/third_party/src/is_utf8

	( if [ ! -e WebserverConfig.h ] ; then cp WebserverConfig.h.sample WebserverConfig.h ; fi )

	@$(MAKE) build

OBJ_LIST = 	obj/src/*.o obj/platform/*.o obj/ssl/*.o obj/python/*.o obj/third_party/src/rb_tree/*.o \
			obj/third_party/src/miniz/*.o obj/third_party/src/is_utf8/*.o 

LINK_OPTS =  -shared   $(PY_LINK) #-Wl,--strip-all -Wl,-O2 -Wl,--gc-sections -Wl,-soname,libWebserver.so

rebuild:
	@$(MAKE) clean
	@$(MAKE) all

build: $(OBJ)
	@echo 'Building Lib : libWebserver.so'

	$(CC)  $(LINK_OPTS) -o lib/libWebserver.so $(OBJ_LIST)

	@echo -e "\033[01;32mexported symbols : "$$(nm -C lib/libWebserver.so | grep " T " | wc -l)"\033[00m"


obj/%.o : %.c
	@echo 'Compiling File : $<'
	@$(CC) $(INCLUDES) $(DEFS) $(FLAGS)  -c -o $@ $<

clean:
	@echo 'Deleting Object Files'

	@rm -rf .lib/obj/*
	@rm -rf obj/*
	@rm -rf lib/*
	@rm -rf *.i *.s *.bc

# http://www.labri.fr/perso/fleury/posts/programming/using-clang-tidy-and-clang-format.html

tidy:
	clang-tidy -config=''  $(LIB_SOURCES) -- \
		-std=c99 -Iinclude/ -I. -Iinclude/intern -Ithird_party/src/rb_tree/ \
		-Iplatform/include/ -Ithird_party/src/is_utf8/ -Ithird_party/src/miniz/

tidy_file:
	clang-tidy -config=''  $(TIDY_FILE) -- \
		-std=c99 -Iinclude/ -I. -Iinclude/intern -Ithird_party/src/rb_tree/ \
		-Iplatform/include/ -Ithird_party/src/is_utf8/ -Ithird_party/src/miniz/
format:
	clang-format -style=file $(SOURCES)


