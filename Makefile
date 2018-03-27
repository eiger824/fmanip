PROGRAM	= fmanip

CC		= gcc

LDFLAGS	=
CFLAGS 	=-c -g -Wall -Wextra -Wpedantic --std=c11

SRC 	= main.c \
		  files.c \
		  utils.c

OBJS 	= main.o \
		  files.o \
		  utils.o

${PROGRAM}: ${OBJS}
	${CC} ${LDFLAGS} $^ -o $@

main.o: main.c defs.h
	${CC} ${CFLAGS} $< -o $@

files.o: files.c files.h defs.h
	${CC} ${CFLAGS} $< -o $@

utils.o: utils.c utils.h defs.h
	${CC} ${CFLAGS} $< -o $@

install:
	install ${PROGRAM} /usr/bin/

uninstall:
	rm -f /usr/bin/fmanip

clean:
	rm -f ${PROGRAM} ${OBJS} *~
