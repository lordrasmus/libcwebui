
CXX_OPTS= -std=c++17 -ggdb3
C_OPTS= -std=c99 -ggdb3

INCLUDES=-I. -I../../lib/ -I../../lib/include/ -I../../lib/include/intern/ -I../../lib/third_party/src/simclist/ \
	-I../../lib/third_party/src/rb_tree/ -I../../lib/third_party/src/is_utf8/ \
	-I../../lib/third_party/src/miniz/ -I../../lib/platform/include/

CXX_FLAGS=${INCLUDES} ${CXX_OPTS}
C_FLAGS=${INCLUDES} ${C_OPTS}

C_FILES= \
	../../lib/src/base64.c \
	../../lib/src/builtinFunctions.c \
	../../lib/src/convert.c \
	../../lib/src/conditions.c \
	../../lib/src/cookie.c \
	../../lib/src/engine.c \
	../../lib/src/engine_functions.c \
	../../lib/src/engine_if.c \
	../../lib/src/engine_tags.c \
	../../lib/src/engine_api_extensions.c \
	../../lib/src/file_manager.c \
	../../lib/src/globals.c \
	../../lib/src/header_parser.c \
	../../lib/src/linked_list.c \
	../../lib/src/log.c \
	../../lib/src/utils.c \
	../../lib/src/server_chunk_functions.c \
	../../lib/src/sha1.c \
	../../lib/src/message_queue.c \
	../../lib/src/header_send.c \
	../../lib/src/system_sockets_container.c \
	../../lib/src/system_sockets.c \
	../../lib/src/system_sockets_send.c \
	../../lib/src/system_memory.c \
	../../lib/src/system_memory_gcc.c \
	../../lib/src/server_error_pages.c \
	../../lib/ssl/openssl.c \
	../../lib/src/server_config.c \
	../../lib/src/system_sockets_epoll.c \
	../../lib/src/session.c \
	../../lib/src/system_sockets_recv.c \
	../../lib/src/engine_parser.c \
	../../lib/src/variable_global.c \
	../../lib/src/variable_render.c \
	../../lib/src/system_sockets_events.c \
	../../lib/src/system.c \
	../../lib/src/server.c \
	../../lib/src/system_file_access.c \
	../../lib/src/system_file_access_fs.c \
	../../lib/src/system_file_access_wnfs.c \
	../../lib/src/system_file_cache.c \
	../../lib/src/system_file_access_utils.c \
	../../lib/src/system_file_access_binary.c \
	../../lib/src/variable_store.c \
	../../lib/src/variable.c \
	../../lib/src/websockets_send_recv.c \
	../../lib/src/websockets_streaming.c \
	../../lib/src/websocket_handler.c \
	../../lib/src/websockets.c \
	../../lib/src/webserver_api_functions.c \
	../../lib/src/reverse_proxy.c \
	../../lib/platform/linux/platform-file-access.c \
	../../lib/platform/linux/platform-sockets.c \
	../../lib/platform/linux/system-unix.c \
	../../lib/third_party/src/is_utf8/is_utf8.c \
	../../lib/third_party/src/miniz/miniz.c \
	../../lib/third_party/src/rb_tree/red_black_tree.c \
	../../lib/third_party/src/rb_tree/stack.c \
	../../lib/third_party/src/rb_tree/misc.c

all: test_cookie

test: test_cookie
	./test_cookie

C_OBJS=$(C_FILES:.c=.o)

%.o: %.c
	gcc ${C_FLAGS} -fno-omit-frame-pointer -c $< -o $@

test_cookie: ${C_OBJS} test_cookie.cpp
	g++ ${CXX_FLAGS} -fno-omit-frame-pointer test_cookie.cpp ${C_OBJS} -o test_cookie \
		-lgtest -lssl -lcrypto -ldl -lpthread -levent -levent_pthreads

# Run with JUnit XML output for CI
test-ci: test_cookie
	./test_cookie --gtest_output=xml:test_results.xml

# List all tests
list: test_cookie
	./test_cookie --gtest_list_tests

# Run filtered tests (usage: make filter FILTER="Cookie*")
filter: test_cookie
	./test_cookie --gtest_filter=${FILTER}

clean:
	rm -f test_cookie test_results.xml ${C_OBJS}

.PHONY: all test test-ci list filter clean
