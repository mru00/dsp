##
## mru, 05-nov-2008
##


## DEBUG
#FLAGS += -g -ggdb

## OPTIMIZE
FLAGS += -O3

## GPROF
#FLAGS += -pg

## GCOV
#FLAGS += --coverage

## ELECTRIC FENCE
#LDFLAGS += -lefence

FFTW_CFLAGS = -include fftw3.h
FFTW_LDFLAGS = -lfftw

FLAGS += -std=c99 -Wall -D_XOPEN_SOURCE=600 -march=prescott
CFLAGS += $(FLAGS) 

LDFLAGS += $(FLAGS) -lm common.o

SRC = convolve.c  vocoder.c synth.c synth2.c song.c \
	bandpass.c midiplay.c midiparse.c midimatch.c midigen.c hist.c 
BIN = $(SRC:.c=)
OBJ = $(SRC:.c=.o)
HDR = common.h


all: $(BIN) $(ETAGS)

flagcache: Makefile
	@if [ "$(CFLAGS) $(LDFLAGS)" != "`cat flagcache`" ] ; then echo "$(CFLAGS) $(LDFLAGS)" > flagcache; fi

$(SRC): flagcache common.o common.h 
common.o midifile.o input.o: flagcache


bandpass song synth synth2 vocoder hist: LDFLAGS += -lfftw -lrfftw
bandpass song synth synth2 vocoder hist: CFLAGS += $(FFTW_CFLAGS)


hist: LDFLAGS += `pkg-config --libs gtk+-2.0` `pkg-config --libs glib-2.0`  `pkg-config --libs gobject-2.0`\
	`pkg-config --libs gthread-2.0`
hist: CFLAGS += `pkg-config --cflags gtk+-2.0`

midimatch: LDFLAGS += -lasound
midimatch: midifile.o input.o


clean:
	rm -fv *.o *~ gmon.out *.gcov *.gcda *.gcdo
	rm -fv $(OBJ) $(BIN)

arch: 
	tar cvzf digspeech.tar.gz cks.txt hp.kernel tp.kernel song.txt \
		$(SRC) $(HDR) common.c play.sh Makefile eminem-the_way_i_am.mid midifile.h midifile.c input.h input.c README


.PHONY: clean all


depend: Makefile
	makedepend -- $(CFLAGS) -- $(SRC) $(HDR) common.c

TAGS:
	etags $(SRC)


## makedepend stuff:

# DO NOT DELETE

convolve.o: /usr/include/signal.h /usr/include/features.h
convolve.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
convolve.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
convolve.o: /usr/include/bits/sigset.h /usr/include/bits/types.h
convolve.o: /usr/include/bits/typesizes.h /usr/include/bits/signum.h
convolve.o: /usr/include/time.h /usr/include/bits/siginfo.h
convolve.o: /usr/include/bits/sigaction.h /usr/include/bits/sigstack.h
convolve.o: /usr/include/sys/ucontext.h /usr/include/bits/sigcontext.h
convolve.o: /usr/include/asm/sigcontext.h /usr/include/asm/types.h
convolve.o: /usr/include/asm-generic/int-ll64.h
convolve.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sigthread.h
convolve.o: common.h /usr/include/stdio.h /usr/include/libio.h
convolve.o: /usr/include/_G_config.h /usr/include/wchar.h
convolve.o: /usr/include/bits/wchar.h /usr/include/gconv.h
convolve.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
convolve.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
convolve.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
convolve.o: /usr/include/getopt.h /usr/include/stdlib.h
convolve.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
convolve.o: /usr/include/sys/types.h /usr/include/math.h
convolve.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
convolve.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
convolve.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
convolve.o: /usr/include/bits/mathcalls.h /usr/include/string.h
convolve.o: /usr/include/assert.h
vocoder.o: /usr/include/fftw3.h /usr/include/stdio.h /usr/include/features.h
vocoder.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
vocoder.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
vocoder.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
vocoder.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
vocoder.o: /usr/include/bits/wchar.h /usr/include/gconv.h
vocoder.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
vocoder.o: common.h /usr/include/unistd.h /usr/include/bits/posix_opt.h
vocoder.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
vocoder.o: /usr/include/getopt.h /usr/include/stdlib.h
vocoder.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
vocoder.o: /usr/include/sys/types.h /usr/include/time.h
vocoder.o: /usr/include/bits/pthreadtypes.h /usr/include/math.h
vocoder.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
vocoder.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
vocoder.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
vocoder.o: /usr/include/bits/mathcalls.h /usr/include/string.h
vocoder.o: /usr/include/assert.h
synth.o: common.h /usr/include/stdio.h /usr/include/features.h
synth.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
synth.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
synth.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
synth.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
synth.o: /usr/include/bits/wchar.h /usr/include/gconv.h
synth.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
synth.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
synth.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
synth.o: /usr/include/getopt.h /usr/include/stdlib.h
synth.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
synth.o: /usr/include/sys/types.h /usr/include/time.h
synth.o: /usr/include/bits/pthreadtypes.h /usr/include/math.h
synth.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
synth.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
synth.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
synth.o: /usr/include/bits/mathcalls.h /usr/include/string.h
synth.o: /usr/include/assert.h
synth2.o: /usr/include/signal.h /usr/include/features.h
synth2.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
synth2.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
synth2.o: /usr/include/bits/sigset.h /usr/include/bits/types.h
synth2.o: /usr/include/bits/typesizes.h /usr/include/bits/signum.h
synth2.o: /usr/include/time.h /usr/include/bits/siginfo.h
synth2.o: /usr/include/bits/sigaction.h /usr/include/bits/sigstack.h
synth2.o: /usr/include/sys/ucontext.h /usr/include/bits/sigcontext.h
synth2.o: /usr/include/asm/sigcontext.h /usr/include/asm/types.h
synth2.o: /usr/include/asm-generic/int-ll64.h
synth2.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sigthread.h
synth2.o: common.h /usr/include/stdio.h /usr/include/libio.h
synth2.o: /usr/include/_G_config.h /usr/include/wchar.h
synth2.o: /usr/include/bits/wchar.h /usr/include/gconv.h
synth2.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
synth2.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
synth2.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
synth2.o: /usr/include/getopt.h /usr/include/stdlib.h
synth2.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
synth2.o: /usr/include/sys/types.h /usr/include/math.h
synth2.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
synth2.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
synth2.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
synth2.o: /usr/include/bits/mathcalls.h /usr/include/string.h
synth2.o: /usr/include/assert.h
song.o: /usr/include/math.h /usr/include/features.h /usr/include/sys/cdefs.h
song.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
song.o: /usr/include/gnu/stubs-32.h /usr/include/bits/huge_val.h
song.o: /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h
song.o: /usr/include/bits/inf.h /usr/include/bits/nan.h
song.o: /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h common.h
song.o: /usr/include/stdio.h /usr/include/bits/types.h
song.o: /usr/include/bits/typesizes.h /usr/include/libio.h
song.o: /usr/include/_G_config.h /usr/include/wchar.h
song.o: /usr/include/bits/wchar.h /usr/include/gconv.h
song.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
song.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
song.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
song.o: /usr/include/getopt.h /usr/include/stdlib.h
song.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
song.o: /usr/include/sys/types.h /usr/include/time.h
song.o: /usr/include/bits/pthreadtypes.h /usr/include/string.h
song.o: /usr/include/assert.h
bandpass.o: /usr/include/signal.h /usr/include/features.h
bandpass.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
bandpass.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
bandpass.o: /usr/include/bits/sigset.h /usr/include/bits/types.h
bandpass.o: /usr/include/bits/typesizes.h /usr/include/bits/signum.h
bandpass.o: /usr/include/time.h /usr/include/bits/siginfo.h
bandpass.o: /usr/include/bits/sigaction.h /usr/include/bits/sigstack.h
bandpass.o: /usr/include/sys/ucontext.h /usr/include/bits/sigcontext.h
bandpass.o: /usr/include/asm/sigcontext.h /usr/include/asm/types.h
bandpass.o: /usr/include/asm-generic/int-ll64.h
bandpass.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sigthread.h
bandpass.o: common.h /usr/include/stdio.h /usr/include/libio.h
bandpass.o: /usr/include/_G_config.h /usr/include/wchar.h
bandpass.o: /usr/include/bits/wchar.h /usr/include/gconv.h
bandpass.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
bandpass.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
bandpass.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
bandpass.o: /usr/include/getopt.h /usr/include/stdlib.h
bandpass.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
bandpass.o: /usr/include/sys/types.h /usr/include/math.h
bandpass.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
bandpass.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
bandpass.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
bandpass.o: /usr/include/bits/mathcalls.h /usr/include/string.h
bandpass.o: /usr/include/assert.h
midiplay.o: /usr/include/getopt.h /usr/include/stdlib.h
midiplay.o: /usr/include/features.h /usr/include/sys/cdefs.h
midiplay.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
midiplay.o: /usr/include/gnu/stubs-32.h /usr/include/bits/waitflags.h
midiplay.o: /usr/include/bits/waitstatus.h /usr/include/sys/types.h
midiplay.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
midiplay.o: /usr/include/time.h /usr/include/bits/pthreadtypes.h
midiplay.o: /usr/include/math.h /usr/include/bits/huge_val.h
midiplay.o: /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h
midiplay.o: /usr/include/bits/inf.h /usr/include/bits/nan.h
midiplay.o: /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h
midiplay.o: common.h /usr/include/stdio.h /usr/include/libio.h
midiplay.o: /usr/include/_G_config.h /usr/include/wchar.h
midiplay.o: /usr/include/bits/wchar.h /usr/include/gconv.h
midiplay.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
midiplay.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
midiplay.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
midiplay.o: /usr/include/string.h /usr/include/assert.h
midiparse.o: /usr/include/stdlib.h /usr/include/features.h
midiparse.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
midiparse.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
midiparse.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
midiparse.o: /usr/include/sys/types.h /usr/include/bits/types.h
midiparse.o: /usr/include/bits/typesizes.h /usr/include/time.h
midiparse.o: /usr/include/bits/pthreadtypes.h /usr/include/math.h
midiparse.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
midiparse.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
midiparse.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
midiparse.o: /usr/include/bits/mathcalls.h common.h /usr/include/stdio.h
midiparse.o: /usr/include/libio.h /usr/include/_G_config.h
midiparse.o: /usr/include/wchar.h /usr/include/bits/wchar.h
midiparse.o: /usr/include/gconv.h /usr/include/bits/stdio_lim.h
midiparse.o: /usr/include/bits/sys_errlist.h /usr/include/unistd.h
midiparse.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
midiparse.o: /usr/include/bits/confname.h /usr/include/getopt.h
midiparse.o: /usr/include/string.h /usr/include/assert.h
midimatch.o: /usr/include/getopt.h /usr/include/math.h
midimatch.o: /usr/include/features.h /usr/include/sys/cdefs.h
midimatch.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
midimatch.o: /usr/include/gnu/stubs-32.h /usr/include/bits/huge_val.h
midimatch.o: /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h
midimatch.o: /usr/include/bits/inf.h /usr/include/bits/nan.h
midimatch.o: /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h
midimatch.o: /usr/include/string.h common.h /usr/include/stdio.h
midimatch.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
midimatch.o: /usr/include/libio.h /usr/include/_G_config.h
midimatch.o: /usr/include/wchar.h /usr/include/bits/wchar.h
midimatch.o: /usr/include/gconv.h /usr/include/bits/stdio_lim.h
midimatch.o: /usr/include/bits/sys_errlist.h /usr/include/unistd.h
midimatch.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
midimatch.o: /usr/include/bits/confname.h /usr/include/stdlib.h
midimatch.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
midimatch.o: /usr/include/sys/types.h /usr/include/time.h
midimatch.o: /usr/include/bits/pthreadtypes.h /usr/include/assert.h
midimatch.o: midifile.h input.h /usr/include/alsa/asoundlib.h
midimatch.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
midimatch.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
midimatch.o: /usr/include/endian.h /usr/include/bits/endian.h
midimatch.o: /usr/include/sys/poll.h /usr/include/bits/poll.h
midimatch.o: /usr/include/errno.h /usr/include/bits/errno.h
midimatch.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
midimatch.o: /usr/include/asm-generic/errno.h
midimatch.o: /usr/include/asm-generic/errno-base.h
midimatch.o: /usr/include/alsa/asoundef.h /usr/include/alsa/version.h
midimatch.o: /usr/include/alsa/global.h /usr/include/alsa/input.h
midimatch.o: /usr/include/alsa/output.h /usr/include/alsa/error.h
midimatch.o: /usr/include/alsa/conf.h /usr/include/alsa/pcm.h
midimatch.o: /usr/include/alsa/rawmidi.h /usr/include/alsa/timer.h
midimatch.o: /usr/include/alsa/hwdep.h /usr/include/alsa/control.h
midimatch.o: /usr/include/alsa/mixer.h /usr/include/alsa/seq_event.h
midimatch.o: /usr/include/alsa/seq.h /usr/include/alsa/seqmid.h
midimatch.o: /usr/include/alsa/seq_midi_event.h
midigen.o: /usr/include/getopt.h /usr/include/stdlib.h
midigen.o: /usr/include/features.h /usr/include/sys/cdefs.h
midigen.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
midigen.o: /usr/include/gnu/stubs-32.h /usr/include/bits/waitflags.h
midigen.o: /usr/include/bits/waitstatus.h /usr/include/sys/types.h
midigen.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
midigen.o: /usr/include/time.h /usr/include/bits/pthreadtypes.h
midigen.o: /usr/include/math.h /usr/include/bits/huge_val.h
midigen.o: /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h
midigen.o: /usr/include/bits/inf.h /usr/include/bits/nan.h
midigen.o: /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h common.h
midigen.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
midigen.o: /usr/include/wchar.h /usr/include/bits/wchar.h
midigen.o: /usr/include/gconv.h /usr/include/bits/stdio_lim.h
midigen.o: /usr/include/bits/sys_errlist.h /usr/include/unistd.h
midigen.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
midigen.o: /usr/include/bits/confname.h /usr/include/string.h
midigen.o: /usr/include/assert.h
hist.o: /usr/include/pthread.h /usr/include/features.h
hist.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
hist.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
hist.o: /usr/include/sched.h /usr/include/bits/types.h
hist.o: /usr/include/bits/typesizes.h /usr/include/time.h
hist.o: /usr/include/bits/sched.h /usr/include/signal.h
hist.o: /usr/include/bits/sigset.h /usr/include/bits/signum.h
hist.o: /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h
hist.o: /usr/include/bits/sigstack.h /usr/include/sys/ucontext.h
hist.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
hist.o: /usr/include/asm/types.h /usr/include/asm-generic/int-ll64.h
hist.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sigthread.h
hist.o: /usr/include/bits/setjmp.h common.h /usr/include/stdio.h
hist.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
hist.o: /usr/include/bits/wchar.h /usr/include/gconv.h
hist.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
hist.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
hist.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
hist.o: /usr/include/getopt.h /usr/include/stdlib.h
hist.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
hist.o: /usr/include/sys/types.h /usr/include/math.h
hist.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
hist.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
hist.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
hist.o: /usr/include/bits/mathcalls.h /usr/include/string.h
hist.o: /usr/include/assert.h
common.o: /usr/include/stdio.h /usr/include/features.h
common.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
common.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
common.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
common.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
common.o: /usr/include/bits/wchar.h /usr/include/gconv.h
common.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
common.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
common.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
common.o: /usr/include/getopt.h /usr/include/stdlib.h
common.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
common.o: /usr/include/sys/types.h /usr/include/time.h
common.o: /usr/include/bits/pthreadtypes.h /usr/include/math.h
common.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
common.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
common.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
common.o: /usr/include/bits/mathcalls.h /usr/include/string.h
common.o: /usr/include/assert.h
common.o: common.h /usr/include/stdio.h /usr/include/features.h
common.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
common.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
common.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
common.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
common.o: /usr/include/bits/wchar.h /usr/include/gconv.h
common.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
common.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
common.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
common.o: /usr/include/getopt.h /usr/include/stdlib.h
common.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
common.o: /usr/include/sys/types.h /usr/include/time.h
common.o: /usr/include/bits/pthreadtypes.h /usr/include/math.h
common.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
common.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
common.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
common.o: /usr/include/bits/mathcalls.h /usr/include/string.h
common.o: /usr/include/assert.h
