
FLAGS=-I../../lib/third_party/src/simclist/ -I../../lib/include

all:
	gcc -O0 -ggdb3 -o test_list_old  $(FLAGS) main.c ../../lib/third_party/src/simclist/simclist.c
	gcc -O0 -ggdb3 -o test_list_new -DUSE_NEW_LIST $(FLAGS) main.c ../../lib/src/linked_list.c


compare: all
	./test_list_old > test_old.txt
	./test_list_new > test_new.txt
	
	diff -u test_old.txt test_new.txt
