

FILES= \
    ../../../lib/src/builtinFunctions.c             ../../../lib/src/convert.c \
    ../../../lib/src/cookie.c                       ../../../lib/src/conditions.c \
    ../../../lib/src/engine.c                       ../../../lib/src/engine_api_extensions.c \
    ../../../lib/src/engine_functions.c             ../../../lib/src/engine_if.c \
    ../../../lib/src/engine_tags.c                  ../../../lib/src/engine_parser.c \
    ../../../lib/src/file_manager.c                 ../../../lib/src/globals.c \
    ../../../lib/src/header_parser.c                ../../../lib/src/header_send.c \
    ../../../lib/src/log.c                          ../../../lib/src/linked_list.c \
    ../../../lib/src/message_queue.c                ../../../lib/src/session.c \
    ../../../lib/src/server.c                       ../../../lib/src/server_config.c \
    ../../../lib/src/server_chunk_functions.c       ../../../lib/src/server_error_pages.c \
    ../../../lib/src/system.c                       ../../../lib/src/system_file_access.c \
    ../../../lib/src/system_file_access_fs.c        ../../../lib/src/system_file_access_wnfs.c \
    ../../../lib/src/system_file_cache.c            ../../../lib/src/system_file_access_utils.c \
    ../../../lib/src/system_file_access_binary.c    ../../../lib/src/system_sockets.c \
	../../../lib/src/system_sockets_container.c     ../../../lib/src/system_sockets_events.c \
	../../../lib/src/system_sockets_epoll.c         ../../../lib/src/system_sockets_send.c \
	../../../lib/src/system_sockets_recv.c          ../../../lib/src/system_memory.c \
	../../../lib/src/system_memory_gcc.c \
	../../../lib/src/utils.c                        ../../../lib/src/variable.c \
	../../../lib/src/variable_global.c              ../../../lib/src/variable_render.c \
	../../../lib/src/variable_store.c               ../../../lib/src/webserver_api_functions.c \
	../../../lib/src/websocket_handler.c            ../../../lib/src/websockets.c \
	../../../lib/src/websockets_send_recv.c         ../../../lib/ssl/openssl.c \
	../../../lib/platform/linux/platform-file-access.c \
	../../../lib/platform/linux/platform-sockets.c \
	../../../lib/platform/linux/system-unix.c \
	../../../lib/python/py_utils.c \
	../../../lib/python/py_plugin.c  \
	../../../lib/python/py_api_functions.c  \
	../../testSite/src/testfunktions.c  \
	../../testSite/src/var_test.c  \
	../../testSite/src/websocket_api.c \
	../../testSite/src/webserver_api_functions.c  \
	../../../lib/third_party/src/rb_tree/red_black_tree.c \
	../../../lib/third_party/src/rb_tree/stack.c \
	../../../lib/third_party/src/rb_tree/misc.c \
	../../../lib/third_party/src/is_utf8/is_utf8.c \
	../../../lib/third_party/src/miniz/miniz.c 


PY_FLAGS = $(shell python-config --includes)
PY_LINK = $(shell python-config --libs)

#PY_FLAGS = -I/usr/include/python3.4
#PY_LINK = -lpython3.4

#PY_FLAGS = -I/usr/include/python2.7
#PY_LINK = -lpython2.7

FLAGS=-I../../../lib/ -I../../../lib/include -I../../../lib/third_party/src/miniz/ -I../../../lib/third_party/src/rb_tree/ \
	-I../../../lib/platform/include/ -I../../../lib/include/intern/  -I../../../lib/third_party/src/is_utf8/ \
	-D_POSIX_SOURCE   -DLINUX -DSINGLE_MAIN -O3 -ggdb3 -flto=12 ${PY_FLAGS} -Wno-unused-result \
	-Werror=return-type -Werror=missing-declarations \
	-DMINIZ_NO_ARCHIVE_APIS -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES

LIBS=-lc -lssl -lcrypto -ldl -lpthread -levent -levent_pthreads -lz -lstdc++

.SILENT:
all: testSite
	echo "C++  main.cpp"
	g++ $(FLAGS) -c main.cpp  -o main.o
	echo "CC   www_css.c www_data.c ../lib/*.c"
	gcc $(FLAGS) -std=gnu99 $(FILES) main.o  www_css.c www_data.c -o main $(LIBS) ${PY_LINK}
	echo "LINK main"

	
testSite: 
	echo "GEN  www_data.c "
	../../../tools/pack_dir.py --dir ../../testSite/www --alias=/ --out www_data.c
	
	echo "GEN  www_css.c "
	../../../tools/pack_dir.py --dir ../../testSite/css --alias=/css/ --out www_css.c

	

clean:
	rm -f *.o main	www_data.c
	
