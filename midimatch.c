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


const float pi = 3.1415;
const int SR = 44100;

#define min(x,y) ((x)<(y)?(x):(y))
#define absolute(x,y) sqrt( (x)*(x) + (y)*(y) )

#define NTONES 128
#define BINLENGTH 256
#define NOSC 70

#define RINGBUFFERLOGSIZE 3
#define RINGBUFFERSIZE (1<<RINGBUFFERLOGSIZE)
#define RINGBUFFERMASK (RINGBUFFERSIZE-1)

static int N = 4096>>1;
static int periodics[NTONES];

static float         *cos_precalc[NTONES];
static float         act_freq[NTONES];
static float         *buffer_f[RINGBUFFERSIZE];
static float         threshold = 15;
static float         weighting_ELC[NTONES];
static short         debug = 0;
static short         print_freqs = 0;
static short         use_harm_comp = 0;
static const int     lo_note = 30;
static const int     hi_note = 80;
static long          absolute_time = 0;
static FILE*         midi_file = NULL;
static long          midi_difftime = 0;
static byte_t        midi_last_status = 0xff;
static int           samples_per_tick;
static const int     midi_bpm = 80;
static const int     midi_timeDivision = 120;

static short current_buffer=0;


// http://en.wikipedia.org/wiki/A-weighting
static float 
ra2(float f) {

  const float a2 = 12200*12200;
  const float b2 = 20.6*20.6;
  const float c2 = 107.7*107.7;
  const float d2 = 737.9*737.9;
  float f2 = f*f;

  float ra = a2 * f2*f2 / ( (f2+b2) * sqrt( (f2+c2)*(f2+d2) ) * (f2+a2) ); 
  return ra;
  return 2.0 + 20*log10(ra);
}

static void 
calculate_frequencies() {

  float hz, rad, rad_per_sample;
  int note, k, per, p;
  for ( note = lo_note; note< hi_note; note++ ) {

	hz = (440.0/32.0) * pow(2.0, (float)(note-9)/12.0);
	rad = hz * 2.0 * pi;
	rad_per_sample = rad / (float)SR;

	per = min(ceil( 2.0 * pi/rad_per_sample), RINGBUFFERSIZE*N);

	cos_precalc[note] = (float*)malloc ( 2* per * sizeof(float));

	for ( p = 0, k = 0; k < per; k++ ) {
	  cos_precalc[note][p++] = cos( rad_per_sample * (float) k );
	  cos_precalc[note][p++] = sin( rad_per_sample * (float) k );
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
  if ( type != midi_last_status ) {
	fwrite(&type, 1, 1, midi_file);
	midi_last_status = type;
  }

  fwrite(&note, 1, 1, midi_file);
  fwrite(&velocity, 1, 1, midi_file);

  fflush(midi_file);
}


static void 
scan_freqs() {

  int note, j, k, p, c;
  float power_re, power_im;
  float power;
  int buffer_index;

  for ( note = lo_note; note<hi_note; note++ ) {

	power_re = 0.0f;
	power_im = 0.0f;

	int periodic = 2 * periodics[note];

	// z-transform
	c = p = 0;
	for ( j = 0; j < RINGBUFFERSIZE; j++ ) {
	  buffer_index = (current_buffer - j) & RINGBUFFERMASK;

	  for ( k = 0; k < N ; k++ ) {
		power_re += buffer_f[buffer_index][k] * cos_precalc[note][p++];
		power_im += buffer_f[buffer_index][k] * cos_precalc[note][p++];
		c ++;

		if ( p == periodic ) p = 0;
	  }
	}


	// power = 20 log10 ( |power| )
	power = 70000*absolute(power_re,power_im)*weighting_ELC[note]/(float)c;

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
		  //		  break;
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
	if (power < (threshold * 0.3) && act_freq[note] ) {

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
  int chunksize = reorder_dword(ftell(midi_file) - track_chunksize_fixup);

  fseek(midi_file,track_chunksize_fixup, SEEK_SET);
  fwrite(&chunksize, 4, 1, midi_file);
}

static void
midi_write_track_end() {
  byte_t b;

  midi_write_var_length ( midi_difftime );
  midi_difftime = 0;

  b = 0xFF; fwrite(&b, 1, 1, midi_file);
  b = 0x2F; fwrite(&b, 1, 1, midi_file);
  midi_write_var_length(0);
}


static void 
usage() {
  fprintf(stderr, "usage: midimatch [-d] [-p] [-h] [-o filename] [-m] [-N number_of_samples] threshold\n");
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
  while ( ( c = getopt(argc, argv, "dphN:o:m") ) != -1) 
	switch (c) {
	case 'd': debug++; break;
	case 'p': print_freqs = 1; break;
	case 'h': use_harm_comp = 1; break;
	case 'N': N = atoi(optarg); break;
	case 'o': midi_filename =strdup(optarg); break;
	case 'm': midi_stdout = 1;  break;
	default: usage();
	}


  if (argc == optind ) 
	usage();

  threshold = atof (argv[optind++]);

  print_prologoue(N, SR);

  buffer = malloc ( N * sizeof(buffer_t) );

  for ( j = 0; j < RINGBUFFERSIZE; j++ ) 
	buffer_f[j] = malloc ( N * sizeof(float) );

  calculate_frequencies();

  if ( midi_stdout ) midi_file = stdout;
  else midi_file = fopen (midi_filename, "wb");

  midi_write_header();
  midi_write_track_header();

  samples_per_tick = (float)SR * ((float)midi_bpm/60) / (float)midi_timeDivision;  

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
	  buffer_f[current_buffer][j] = ((float)buffer[j]-128)/128.0;
	}
	current_buffer = (current_buffer+1) & RINGBUFFERMASK;


	div_t r = div(rest+ N, samples_per_tick);
	midi_difftime += r.quot;
	rest = r.rem;

	absolute_time += N;


	scan_freqs();

  }


  free(buffer);
  for ( j = 0; j < RINGBUFFERSIZE; j++ ) 
	free(buffer_f[j]);

  for (i = 0; i< NTONES; i++ ) {
	free(cos_precalc[i]);
  }

  midi_write_track_end();
  midi_fixup_chunksize();
  fclose(midi_file);

  print_epilogue();

  return 0;
}
