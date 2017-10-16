PROGRAM=fmanip

CC=gcc

LDFLAGS=
CFLAGS ?=-c -Wall -Wextra -Wpedantic

SRC=main.c

OBJ=main.o


${PROGRAM}: ${OBJ}
	${CC} ${LDFLAGS} ${OBJ} -o $@

${OBJ}: ${SRC}
	${CC} ${CFLAGS} ${SRC} -o $@

install:
	install ${PROGRAM} /usr/bin/

uninstall:
	rm -f /usr/bin/fmanip

clean:
	rm -f ${PROGRAM} ${OBJ} *~
