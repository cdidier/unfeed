BIN = unfeed

CFLAGS = -g -Wall -W -Wpointer-arith -Wbad-function-cast -Wno-unused-parameter
LDFLAGS = -lexpat

SRCS = config.c fetch.c filter.c format.c main.c parser.c tools.c \
	output_cmd.c output_html.c output_mail.c output_text.c
OBJS = ${SRCS:.c=.o}
MAN = ${BIN}.1

all: ${BIN}

${BIN}: ${OBJS}
	${CC} ${LDFLAGS} -o ${BIN} ${OBJS}

.c.o:
	${CC} ${CFLAGS} -o $@ -c $<

${BIN}.cat1: ${MAN}
	nroff -Tascii -mandoc ${MAN} > ${BIN}.cat1

clean:
	rm -f ${BIN} ${BIN}.core ${OBJS}
