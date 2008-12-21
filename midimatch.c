/*
 * transposes PCM audio to MIDI
 *
 *
 * mru, 10-nov-2008
 *
 */

#include <getopt.h>

#include <math.h>
#include <string.h>
#include "common.h"


const int SR = 44100;

#define min(x,y) ((x)<(y)?(x):(y))
#define absolute(x,y) sqrt( (x)*(x) + (y)*(y) )

#define NTONES 128
#define BINLENGTH 256
#define NOSC 70

#define RINGBUFFERLOGSIZE 2
#define RINGBUFFERSIZE (1<<ringsize)
#define RINGBUFFERMASK (RINGBUFFERSIZE-1)

static int N = 4096>>1;
static int periodics[NTONES];

static double         *cos_precalc[NTONES];
static double         act_freq[NTONES];
static double         **buffer_f;
static double         threshold = 15.0;
static double         weighting_ELC[NTONES];
static short         debug = 0;
static short         print_freqs = 0;
static short         use_harm_comp = 0;
static const int     lo_note = 30;
static const int     hi_note = 100;
static long          absolute_time = 0;
static FILE*         midi_file = NULL;
static long          midi_difftime = 0;
static byte_t        midi_running_status = 0xff;
static int           samples_per_tick;
static const double  midi_bpm = 120.0;
static const int     midi_timeDivision = 120;
static double        gain = 1.0;
static double        hysteresis = 7;
static short         ringsize = 0;
static short current_buffer=0;


// http://en.wikipedia.org/wiki/A-weighting
static double 
ra2(double f) {

  const double a2 = 12200*12200;
  const double b2 = 20.6*20.6;
  const double c2 = 107.7*107.7;
  const double d2 = 737.9*737.9;
  double f2 = f*f;

  double ra = a2 * f2*f2 / ( (f2+b2) * sqrt( (f2+c2)*(f2+d2) ) * (f2+a2) ); 
  return ra;
  return 2.0 + 20*log10(ra);
}

static void 
calculate_frequencies() {

  double hz, rad, rad_per_sample;
  int note, k, per, p;
  for ( note = lo_note; note< hi_note; note++ ) {

	hz = midi_note_to_herz(note);
	rad = herz_to_radians(hz);
	rad_per_sample = rad / (double)SR;

	per = min(ceil( 2.0 * pi/rad_per_sample), RINGBUFFERSIZE*N);

	cos_precalc[note] = (double*)malloc ( 2* per * sizeof(double));

	for ( p = 0, k = 0; k < per; k++ ) {
	  cos_precalc[note][p++] = cos( rad_per_sample * (double) k );
	  cos_precalc[note][p++] = sin( rad_per_sample * (double) k );
	}

	weighting_ELC[note] = ra2(hz);

	periodics[note] = per;

	if (print_freqs) 
	  fprintf(stderr, 
			  "note: %-3d freq_hz:%-5.2f th:%-5.1f periodics:%i\n", 
			  note, hz, weighting_ELC[note], per);
  }
}


//http://www.omega-art.com/midi/mfiles.html
static void 
midi_write_var_length(unsigned long number) {
  byte_t b;
  
  unsigned long buffer;
  buffer = number & 0x7f;
  while ((number >>= 7) > 0) {
	buffer <<= 8;
	buffer |= 0x80 + (number & 0x7f);
  }
  while (1) {
	b = buffer & 0xff;
	fwrite(&b, 1, 1, midi_file);
	
	if (buffer & 0x80) buffer >>= 8;
	else break;
  }
}

static void 
midi_write_note_event(byte_t type, byte_t note, byte_t velocity) {

  midi_write_var_length ( midi_difftime );
  midi_difftime = 0;

  // running status
  if ( type != midi_running_status ) {
	fwrite(&type, 1, 1, midi_file);
	midi_running_status = type;
  }

  fwrite(&note, 1, 1, midi_file);
  fwrite(&velocity, 1, 1, midi_file);

  fflush(midi_file);
}


static void 
scan_freqs() {

  int note, j, k, p, c;
  double power_re, power_im;
  double power;
  int buffer_index;

  for ( note = lo_note; note<hi_note; note++ ) {

	power_re = 0.0f;
	power_im = 0.0f;

	int periodic = 2 * periodics[note];

	// z-transform
	c = p = 0;
	for ( j = 0; j < RINGBUFFERSIZE; j++ ) {
	  buffer_index = (current_buffer + j) & RINGBUFFERMASK;

	  for ( k = 0; k < N ; k++ ) {
		power_re += buffer_f[buffer_index][k] * cos_precalc[note][p++];
		power_im += buffer_f[buffer_index][k] * cos_precalc[note][p++];
		c ++;

		if ( p == periodic ) p = 0;
	  }
	}


	// power = 20 log10 ( |power| )
	power = 70000*absolute(power_re,power_im);
	power *= weighting_ELC[note];
	power /= (double)c;
	power /= gain;

	if (debug>1) 
	  fprintf(stderr, "midimatch: (%ld) note:%-3d power:%-5.0f "
			  "weighting:%-5.0f re:%-5.0f im:%-5.0f periodics:%d\n", 
			  absolute_time, note, power, weighting_ELC[note], 
			  power_re, power_im, periodics[note]);


/* 	// power = 20 log10 ( |power| ) */
/* 	power =50+ 20 * log10 ( absolute(power_re,power_im)  ); */

/* 	// equal loudness compensation */
/* 	power += weighting_ELC[note]; */

	// harmonic compensation
	if ( use_harm_comp ) {
	  j = note-12;
	  while ( j > lo_note ) {
		if (act_freq[j]) {
		  power = -10000.0;
		  break;
		}
		j -= 12;
	  }
	}

	// note on
	if ( power > threshold && !act_freq[note] ) {

	  if (debug>0) 
		fprintf(stderr, "midimatch: (%ld, %ld) note on:  note:%-3d p:%.2f\n", 
				absolute_time, midi_difftime, note-12, power);
	  
	  act_freq[note] = 1;
	  midi_write_note_event(0x90 | 0x09, note-12, 100);

	}

	// note off
	if (power < (threshold / hysteresis) && act_freq[note] ) {

	  if (debug>0) 
		fprintf(stderr, "midimatch: (%ld, %ld) note off: note:%-3d p:%.2f\n", 
				absolute_time, midi_difftime, note-12, power);

	  act_freq[note] = 0;
	  midi_write_note_event(0x80 | 0x09, note-12, 100);

	}
  }
}


static void 
midi_write_header() {

  assert (midi_file != NULL);
  int chunksize = reorder_dword(6);
  short formattype = reorder_word(1);
  short numberoftracks  = reorder_word(1);
  short timedivision= reorder_word(midi_timeDivision);

  fwrite("MThd", 4, 1, midi_file);
  fwrite(&chunksize, 4, 1, midi_file);
  fwrite(&formattype, 2, 1, midi_file);
  fwrite(&numberoftracks, 2, 1, midi_file);
  fwrite(&timedivision, 2, 1, midi_file);
}

long track_chunksize_fixup;

static void 
midi_write_track_header() {
  assert (midi_file != NULL);

  int chunksize = reorder_dword(60000);

  fwrite("MTrk", 4, 1, midi_file);
  track_chunksize_fixup = ftell(midi_file);
  fwrite(&chunksize, 4, 1, midi_file);
}

static void 
midi_fixup_chunksize() {

  long pos = ftell(midi_file);

  int chunksize = reorder_dword(pos - track_chunksize_fixup - 4);

  fseek(midi_file,track_chunksize_fixup, SEEK_SET);
  fwrite(&chunksize, 4, 1, midi_file);

  fseek(midi_file, pos, SEEK_SET);
}

static void
midi_write_track_end() {
  byte_t b;

  midi_write_var_length ( 0 );
  midi_difftime = 0;

  b = 0xFF; fwrite(&b, 1, 1, midi_file);
  b = 0x2F; fwrite(&b, 1, 1, midi_file);
  midi_write_var_length(0);
}


static void 
usage() {
  fprintf(stderr, 
		  "usage: midimatch OPTIONS threshold\n"
		  "  [-d]             increase debug level\n"
		  "  [-p]             print frequency table\n"
		  "  [-h]             use harmonic compensation\n"
		  "  [-o filename]    write midi data to filename, matched.midi is default\n"
		  "  [-g gain]        apply gain on input (float, applied multiplicaive)\n"
		  "  [-m]             write midi to stdout\n"
		  "  [-N numberofsamples] input buffer size, 2^ringsize*N ~ 4000\n"
		  "  [-k hysteresis]  float, ~10\n"
		  "  [-r ringsize]    2 or 3, will be used as 2^ringsize\n");
  abort();
}

int 
main(int argc, char** argv) {


  int i, rd, j, rest = 0;
  long bytes = 0;
  char* midi_filename = "matched.midi";
  short midi_stdout = 0;
  buffer_t* buffer;


  int c;
  while ( ( c = getopt(argc, argv, "dphN:o:mg:k:r:") ) != -1) 
	switch (c) {
	case 'd': debug++; break;
	case 'p': print_freqs = 1; break;
	case 'h': use_harm_comp = 1; break;
	case 'N': N = atoi(optarg); break;
	case 'g': gain = atof(optarg); break;
	case 'o': midi_filename =strdup(optarg); break;
	case 'm': midi_stdout = 1;  break;
	case 'k': hysteresis = atof(optarg); break;
	case 'r': ringsize = atoi(optarg); break;
	default: usage();
	}


  if (argc == optind ) 
	usage();

  threshold = atof (argv[optind++]);

  print_prologoue(N, SR);

  buffer = (buffer_t*)malloc ( N * sizeof(buffer_t) );

  buffer_f = (double**)malloc ( RINGBUFFERSIZE * sizeof(double*) );

  for ( j = 0; j < RINGBUFFERSIZE; j++ ) 
	buffer_f[j] = malloc ( N * sizeof(double) );

  calculate_frequencies();

  if ( midi_stdout ) midi_file = stdout;
  else midi_file = fopen (midi_filename, "wb");

  midi_write_header();
  midi_write_track_header();

  samples_per_tick = (double)SR*midi_bpm / (60.0*(double)midi_timeDivision);

  while ( 1 ) { 
	// read sample
	rd = read(0, buffer, N* sizeof(buffer_t)); 
	bytes += rd;

	// eof
	if ( rd != N*sizeof(buffer_t) )  {
	  fprintf(stderr, "%li bytes processed, %d read, exiting (midimatch)\n", 
			  bytes, rd);
	  break;
	}

	for ( j = 0; j < N; j++ ) {
	  buffer_f[current_buffer][N-j-1] = gain*((double)buffer[j]-128);
	}
	current_buffer = (current_buffer-1) & RINGBUFFERMASK;


	div_t r = div(rest+ N, samples_per_tick);
	midi_difftime += r.quot;
	rest = r.rem;

	absolute_time += N;

	scan_freqs();
  }


  free(buffer);
  for ( j = 0; j < RINGBUFFERSIZE; j++ ) 
	free(buffer_f[j]);
  free(buffer_f);

  for (i = 0; i< NTONES; i++ ) {
	free(cos_precalc[i]);
  }

  midi_write_track_end();
  midi_fixup_chunksize();
  fclose(midi_file);

  print_epilogue();

  return 0;
}
