OPTS=-fsanitize=address -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Werror -std=c17 -Wpedantic -O0 -g

all: hw4

hw4: hw4.o
	gcc $(OPTS) $^ -o $@

hw4.o: hw4.c
	gcc $(OPTS) -c $<

clean:
	rm -rf *.o hw4
