BIN= unfeed
CC= pcc

CFLAGS=-ggdb -Wall -W -Wtraditional -Wpointer-arith -Wbad-function-cast
LDFLAGS=-lutil -lexpat

SRCS= config.c fetch.c format.c main.c output.c parser.c tools.c
OBJS= ${SRCS:.c=.o}

all: ${BIN}

${BIN}: ${OBJS}
	${CC} ${LDFLAGS} -o ${BIN} ${OBJS}

.c.o:
	${CC} ${CFLAGS} -o $@ -c $<

clean:
	rm -f ${BIN} ${BIN}.core ${OBJS}
