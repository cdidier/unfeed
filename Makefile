BIN= unfeed

CFLAGS=-g -Wall -W -Wpointer-arith -Wbad-function-cast
LDFLAGS=-lexpat

SRCS= config.c fetch.c filter.c format.c main.c parser.c tools.c \
	output_html.c output_mail.c output_text.c
OBJS= ${SRCS:.c=.o}

all: ${BIN}

${BIN}: ${OBJS}
	${CC} ${LDFLAGS} -o ${BIN} ${OBJS}

.c.o:
	${CC} ${CFLAGS} -o $@ -c $<

clean:
	rm -f ${BIN} ${BIN}.core ${OBJS}
