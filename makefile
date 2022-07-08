all:
	gcc -Wall -std=c11 -g obj_code.c -o obj_code
	./obj_code
	gcc -Wall -std=c11 -g main.c
