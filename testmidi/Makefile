


WAVS = $(shell ls *.wav)
MIDIS = $(WAVS:.wav=.midi)
PLOTS = $(MIDIS:.midi=.m)

ARCHINCLUDE = $(WAVS)

MIDIMATCH = ../midimatch




all: $(MIDIS) $(PLOTS)
mp3s: $(WAVS:.wav=.mp3)


%.m: %.log
	@grep freqs $< > $@
	@echo "plot ( freqs(1,:), freqs(2,:)); pause" >> $@


%.log: %.midi


%.opts:
	@touch $@

%.midi: %.wav %.opts $(MIDIMATCH)
	sox $< -t raw -f -4 -r 44100 -c 1 - | $(MIDIMATCH) -vv -t 9 -s -i stdin $(shell cat $*.opts) -o $@ 2> $*.log

%.mp3: %.wav
	sox $< $@


arch: $(ARCHINCLUDE:.wav=.mp3) $(ARCHINCLUDE:.wav=.midi) $(ARCHINCLUDE:.wav=.opts)
	tar czf testmidi.tar.gz Makefile $^

clean:
	-rm -v *.midi *.log *.m

.PHONY: clean all mp3s arch
