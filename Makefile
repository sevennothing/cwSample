
PREFIX=/usr/local

#PLATFORM= "arm"

ifeq ($(PLATFORM), "arm")
CC=arm-linux-gcc
else
SOUND_CARD= "alas"
endif

CFLAGS+=-DENABLE_DEUG_SIM
CFLAGS+=-DSELF_TEST_FOR_PIPE
#CFLAGS+=-DPADDING_INVALID_CODE

DFLAGS+=-lm -lrt -lpthread


ifeq ($(SOUND_CARD), "alas")
CFLAGS+=-DUSE_ALAS_DRIVER
DFLAGS+=-lasound
endif

ALL: cws

install: ALL
	        install -sc cws ${PREFIX}/bin

cws: cw_gpio.o main.o trans.o process.o pcm.o play.o
	        ${CC} ${CFLAGS} -o cws ${DFLAGS}  $^


cw_gpio.o: cw_gpio.c cw_gpio.h
	        ${CC} ${CFLAGS} -c cw_gpio.c

trans.o: trans.c trans.h
	        ${CC} ${CFLAGS} -c trans.c

process.o: process.c trans.h process.h
	        ${CC} ${CFLAGS} -c process.c

pcm.o: pcm.c pcm.h
	        ${CC} ${CFLAGS} -c pcm.c
play.o: play.c pcm.h
	        ${CC} ${CFLAGS} -c play.c

morse.o: morse.c morse.h cw.h
	        ${CC} ${CFLAGS} -c morse.c

main.o: main.c process.h cw_gpio.h
	        ${CC} ${CFLAGS} -c main.c

clean:
	        -rm -f *.o cws
