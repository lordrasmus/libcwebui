all:
	gcc -o base64_test main.c ../../lib/src/base64.c ../../lib/src/system_memory.c ../../lib/src/system_memory_gcc.c \
	../../lib/src/linked_list.c ../../lib/src/system_sockets_events.c \
	../../lib/src/variable_store.c ../../lib/src/server_chunk_functions.c ../../lib/src/variable.c \
	../../lib/src/log.c ../../lib/src/utils.c ../../lib/third_party/src/miniz/miniz.c \
	../../lib/third_party/src/rb_tree/red_black_tree.c ../../lib/third_party/src/rb_tree/stack.c ../../lib/third_party/src/rb_tree/misc.c \
	../../lib/platform/platform.c ../../lib/platform/platform-file-access.c \
	-lpthread -levent -levent_pthreads -lssl -lcrypto \
	-I. -I../../lib/include/ -I../../lib/platform/include/ \
		-I../../lib/third_party/src/rb_tree/ -I../../lib/third_party/src/is_utf8/ -I../../lib/third_party/src/miniz/
