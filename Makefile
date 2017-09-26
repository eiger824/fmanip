fmanip: main.c
	gcc -Wall -Wextra -Wpedantic main.c -o fmanip

clean:
	rm -f fmanip *~
