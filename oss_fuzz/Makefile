

FLAGS= -DLINUX -I. -I../lib/include/ -I../lib/include/intern/ -I../lib/third_party/src/simclist/ \
	-I../lib/third_party/src/rb_tree/ -I../lib/platform/include/ -I../lib/third_party/src/is_utf8/ \
	

FILES= stubs.c ../lib/src/convert.c ../lib/src/variable_store.c ../lib/src/header_parser.c ../lib/src/system_memory.c ../lib/platform/linux/system-unix.c \
	../lib/src/utils.c ../lib/src/linked_list.c ../lib/src/server_chunk_functions.c ../lib/src/variable.c \
	../lib/src/cookie.c ../lib/src/websockets.c ../lib/src/engine_api_extensions.c ../lib/third_party/src/rb_tree/red_black_tree.c \
	../lib/third_party/src/rb_tree/stack.c ../lib/third_party/src/rb_tree/misc.c ../lib/src/message_queue.c ../lib/src/header_send.c \
	../lib/platform/linux/platform-file-access.c \
	../lib/src/system_sockets_send.c ../lib/platform/linux/platform-sockets.c ../lib/src/log.c ../lib/src/server_error_pages.c \
	../lib/ssl/openssl.c ../lib/src/server_config.c ../lib/src/system_sockets_epoll.c ../lib/src/session.c \
	../lib/src/system_sockets_recv.c ../lib/src/websockets_send_recv.c ../lib/src/conditions.c ../lib/src/globals.c \
	../lib/src/engine_parser.c ../lib/src/variable_global.c ../lib/src/variable_render.c  \
	../lib/src/websocket_handler.c ../lib/src/server.c ../lib/src/system_file_access.c ../lib/src/system_file_access_fs.c \
	../lib/src/system_file_access_wnfs.c ../lib/src/system_file_cache.c ../lib/src/system_file_access_utils.c \
	../lib/src/builtinFunctions.c ../lib/third_party/src/miniz/tinfl.c ../lib/src/engine.c ../lib/src/file_manager.c \
	../lib/src/engine_functions.c ../lib/src/engine_if.c ../lib/src/engine_tags.c \
	../lib/third_party/src/is_utf8/is_utf8.c \
	../lib/python/py_api_functions.c ../lib/python/py_plugin.c ../lib/python/py_utils.c

#../lib/src/system.c  ../lib/src/system_sockets_container.c ../lib/src/system_sockets.c ../lib/src/system_sockets_events.c
	

all:
	$(CC)  $(CFLAGS) $(FILES) $(FLAGS)  -c -O0 -ggdb3  -fno-omit-frame-pointer
	$(CXX) $(CXXFLAGS) -std=c++11 *.o main.c $(FLAGS) -o $(OUT)/header_fuzzer -pthread -lFuzzingEngine 

test: 
	$(CC)  $(CFLAGS) $(FILES) $(FLAGS)  -c -O0 -ggdb3  -fno-omit-frame-pointer -fsanitize=address
	$(CXX) $(CXXFLAGS) -O0 -ggdb3 -std=c++11 *.o main_tester.c $(FLAGS) -o header_fuzzer_tester -pthread -ldl -fno-omit-frame-pointer -fsanitize=address
	
test_clang: 
	clang  $(CFLAGS) $(FILES) $(FLAGS)  -c -O0 -ggdb3  -fno-omit-frame-pointer -fsanitize=address
	clang++ $(CXXFLAGS) -O0 -ggdb3 -std=c++11 *.o main_tester.c $(FLAGS) -o header_fuzzer_tester -pthread -ldl -fno-omit-frame-pointer -fsanitize=address
	
clean:
	rm -f *.o
