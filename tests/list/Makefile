
FLAGS=-I../../lib/include/ -I../../lib/include/intern -I.

all: compare

build:
	gcc -O0 -ggdb3 -o test_list $(FLAGS) main.c ../../lib/src/linked_list.c


compare: build
	./test_list > test.txt
	
	diff -u test_expected.txt test.txt
