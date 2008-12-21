##
## mru, 05-nov-2008
##


#USE_PG = -pg
USE_PG = 

DEBUG = -g -ggdb
#DEBUG = -O3


CFLAGS += $(DEBUG) -std=c99 -Wall  $(USE_PG) -D_XOPEN_SOURCE=600
LDFLAGS += -lm common.o


SRC = convolve.c  vocoder.c synth.c synth2.c song.c \
	bandpass.c midiplay.c midiparse.c midimatch.c midigen.c hist.c
BIN = $(SRC:.c=)
OBJ = $(SRC:.c=.o)
HDR = common.h


all: $(BIN) $(ETAGS)

$(SRC): common.o common.h Makefile

bandpass: LDFLAGS += -lfftw -lrfftw
song: LDFLAGS += -lfftw -lrfftw
synth: LDFLAGS += -lfftw -lrfftw
synth2: LDFLAGS += -lfftw -lrfftw
vocoder: LDFLAGS += -lfftw -lrfftw
hist: LDFLAGS += `pkg-config --libs gtk+-2.0` -lrfftw -lfftw \
	`pkg-config --libs glib-2.0`  `pkg-config --libs gobject-2.0`\
	`pkg-config --libs gthread-2.0`
hist: CFLAGS += `pkg-config --cflags gtk+-2.0`


ETAGS: $(SRC) $(HDR)
	etags *.c *.h

test2: convolve
	sox mm.wav -1 -r 22050 -c 1 -t raw - | ./convolve tp.kernel | play -1 -r 22050 -c 1 -t raw -s  -

test3: convolve
	sox mm.wav -1 -r 22050 -c 1 -t raw - | ./convolve hp.kernel | play -1 -r 22050 -c 1 -t raw -s  -

test4: convolve hist
	sox mm.wav -1 -r 22050 -c 1 -t raw - | ./hist in | ./convolve hp.kernel | ./hist out | play -1 -r 22050 -c 1 -t raw -s  -

clean:
	rm -fv *.o *~ gmon.out
	rm -fv $(OBJ) $(BIN)

arch: all
	tar cvzf digspeech.tar.gz cks.txt hp.kernel tp.kernel song.txt \
		$(SRC) $(HDR) common.c play.sh Makefile eminem-the_way_i_am.mid README


.PHONY: clean all test


##