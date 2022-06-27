
FLAGS=-I../../lib/include/ -I../../lib/include/intern -I.

all: compare

build:
	gcc  -O3 -ggdb3 -o test_list $(FLAGS) main.c ../../lib/src/linked_list.c -fsanitize=address


compare: build
	./test_list > test.txt
	
	diff -u test_expected.txt test.txt
