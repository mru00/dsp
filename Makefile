#CFLAGS += -g -ggdb -std=c99 -Wall -D__USE_XOPEN
CFLAGS += -O3 -std=c99 -Wall -D__USE_XOPEN


CFLAGS_HIST = $(CFLAGS) `pkg-config --cflags gtk+-2.0`
LDFLAGS_HIST =  $(LDFLAGS) `pkg-config --libs gtk+-2.0` -lrfftw -lfftw \
	`pkg-config --libs glib-2.0`  `pkg-config --libs gobject-2.0`\
	`pkg-config --libs gthread-2.0`

LDFLAGS_VOCODER = $(LDFLAGS) -lrfftw -lfftw
LDFLAGS_SYNTH = $(LDFLAGS) -lrfftw -lfftw


SRC = convolve.c hist.c vocoder.c synth.c synth2.c song.c \
	bandpass.c midiplay.c midiparse.c midimatch.c midigen.c
BIN = $(SRC:.c=)
OBJ = $(SRC:.c=.o)
HDR = common.h

all: $(BIN) $(ETAGS)

midimatch: midimatch.c 
	$(CC) $(CFLAGS) $<  $(LDFLAGS_SYNTH) -lm -o $@

midigen: midigen.c 
	$(CC) $(CFLAGS) $<  $(LDFLAGS_SYNTH) -lm -o $@

midiplay: midiplay.c 
	$(CC) $(CFLAGS) $<  $(LDFLAGS) -lm -o $@

midiparse: midiparse.c 
	$(CC) $(CFLAGS) $<  $(LDFLAGS) -lm -o $@

bandpass: bandpass.c 
	$(CC) $(CFLAGS) $<  $(LDFLAGS_SYNTH) -o $@

song: song.c 
	$(CC) $(CFLAGS) $< $(LDFLAGS_SYNTH) -o $@

synth2: synth2.c 
	$(CC) $(CFLAGS) $? $(LDFLAGS_SYNTH) -o $@

synth: synth.c 
	$(CC) $(CFLAGS) $? $(LDFLAGS_SYNTH) -o $@

vocoder: vocoder.c 
	$(CC) $(CFLAGS) $? $(LDFLAGS_VOCODER) -o $@

convolve: convolve.c 
	$(CC) $(CFLAGS) $? $(LDFLAGS) -o $@

hist: hist.c 
	$(CC) $(CFLAGS_HIST) $? $(LDFLAGS_HIST) -o $@

ETAGS: $(SRC) $(HDR)
	etags *.c *.h

test2: convolve
	sox mm.wav -1 -r 22050 -c 1 -t raw - | ./convolve tp.kernel | play -1 -r 22050 -c 1 -t raw -s  -

test3: convolve
	sox mm.wav -1 -r 22050 -c 1 -t raw - | ./convolve hp.kernel | play -1 -r 22050 -c 1 -t raw -s  -

test4: convolve hist
	sox mm.wav -1 -r 22050 -c 1 -t raw - | ./hist in | ./convolve hp.kernel | ./hist out | play -1 -r 22050 -c 1 -t raw -s  -

clean:
	rm -fv *.o *~
	rm -fv $(OBJ) $(BIN)

arch: all
	tar cvzf digspeech.tar.gz cks.txt hp.kernel tp.kernel song.txt \
		$(SRC) $(HDR) play.sh Makefile eminem-the_way_i_am.mid README


.PHONY: clean all test
