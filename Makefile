all: fatcat
fatcat: *.c
	#gcc -O2 -Wall -Wextra -fsanitize=undefined -fsanitize=address $? -o fatcat
	gcc -Wall -Wextra $? -o fatcat
clean:
	rm -f *.o
	rm -f fatcat
