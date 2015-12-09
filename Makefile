CC = gcc
CFLAGS = -march=native -O2 -Wall -Wextra -Wno-unused-result
CDEBUGFLAGS = -fsanitize=undefined -fsanitize=address -fsanitize=leak

all: fatcat
fatcat: *.c
	@${CC} ${CFLAGS} $? -o fatcat
debug: *.c
	@${CC} ${CFLAGS} ${CDEBUGFLAGS} $? -o fatcat
clean:
	rm -f *.o
	rm -f fatcat
