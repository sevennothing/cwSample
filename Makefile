
PREFIX=/usr/local

ALL: cws

install: ALL
	        install -sc cws ${PREFIX}/bin

cws: cw_gpio.o main.o trans.o process.o
	        ${CC} ${CFLAGS} -o cws -lrt $^


cw_gpio.o: cw_gpio.c cw_gpio.h
	        ${CC} ${CFLAGS} -c cw_gpio.c

trans.o: trans.c trans.h
	        ${CC} ${CFLAGS} -c trans.c

process.o: process.c trans.h process.h
	        ${CC} ${CFLAGS} -c process.c

main.o: main.c process.h
	        ${CC} ${CFLAGS} -c main.c

clean:
	        -rm -f *.o cws
