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



#define min(x,y) ((x)<(y)?(x):(y))
#define absolute(x,y) sqrt( (x)*(x) + (y)*(y) )

// number of midi notes 
#define NTONES 128     

#define RINGBUFFERSIZE (1<<ringsize)
#define RINGBUFFERMASK (RINGBUFFERSIZE-1)

// buffers & statistics
static int           periodics[NTONES];
static double        *cos_precalc[NTONES];
static double        act_freq[NTONES];
static double        **buffer_f;
static double        threshold = 35.0;
static double        weighting_ELC[NTONES];
static unsigned long absolute_time = 0;
static unsigned long stats_note_ons = 0;

// config with default values
static int           N = 128;
static int           SR = 44100;
static short         debug = 0;
static short         print_freqs = 0;
static short         use_harm_comp = 1;
static const int     lo_note = 30;
static const int     hi_note = 100;
static double        gain = 1.0;
static double        hysteresis = 7;
static short         ringsize = 4;
static short         current_buffer=0;
static short         use_local_maximum = 0;
static char*         midi_filename = "matched.midi";

// midi config
static FILE*         midi_file = NULL;
static long          midi_difftime = 0;
static byte_t        midi_running_status = 0xff;
static short         midi_channel = 2;
static long          midi_track_chunksize_fixup;
static const double  midi_bpm = 120.0;
static const int     midi_timeDivision = 120;


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
precalculate() {

  double rad_per_sample;
  int note, k, per, p;
  for ( note = lo_note; note< hi_note; note++ ) {

	rad_per_sample = midi_note_to_radians_per_sample(SR,note);

	per = ceil( 2.0 * pi/rad_per_sample);

	cos_precalc[note] = (double*)malloc ( 2* per * sizeof(double));

	for ( p = 0, k = 0; k < per; k++ ) {
	  cos_precalc[note][p++] = cos( rad_per_sample * (double) k );
	  cos_precalc[note][p++] = sin( rad_per_sample * (double) k );
	}

	weighting_ELC[note] = ra2(midi_note_to_hertz(note));

	periodics[note] = per;

	if (print_freqs) 
	  fprintf(stderr, 
			  "note: %-3d weighting:%-5.1f periodics:%i\n", 
			  note,  weighting_ELC[note], per);
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
note_on(int note, double power) {

  if (debug>0) fprintf(stderr, ".");
  if (debug>1) 
	fprintf(stderr, "midimatch: (%ld, %ld) note on:  note:%-3d p:%.2f\n", 
			absolute_time, midi_difftime, note, power);
  
  act_freq[note] = 1;
  midi_write_note_event(0x90 | midi_channel, note, 100);
  stats_note_ons ++;
}


static void
note_off(int note, double power) {
  if (debug>1) 
	fprintf(stderr, "midimatch: (%ld, %ld) note off: note:%-3d p:%.2f\n", 
			absolute_time, midi_difftime, note, power);
  
  act_freq[note] = 0;
  midi_write_note_event(0x80 | midi_channel, note, 100);
}

static double
get_power(int note) {
  double power_re = 0.0f;
  double power_im = 0.0f;
  double power = 0.0f;

  int p, c, j, k;
  int buffer_index;
  int periodic = 2 * periodics[note];

  // z-transform
  c = p = 0;
  for ( j = 0; j < RINGBUFFERSIZE; j++ ) {
	buffer_index = (current_buffer + j + 1) & RINGBUFFERMASK;
	
	for ( k = 0; k < N ; k++ ) {
	  power_re += buffer_f[buffer_index][k] * cos_precalc[note][p++];
	  power_im += buffer_f[buffer_index][k] * cos_precalc[note][p++];
	  c ++;
	  
	  if ( p == periodic ) p = 0;
	}
  }

  // power = 20 log10 ( |power| )
  power = 10*absolute(power_re,power_im);
  power *= weighting_ELC[note];
  power /= (double)c;
  power /= gain;


  if (debug>1) 
	fprintf(stderr, "midimatch: (%ld) note:%-3d power:%-5.0f "
			"weighting:%-5.0f re:%-5.0f im:%-5.0f periodics:%d\n", 
			absolute_time, note, power, weighting_ELC[note], 
			power_re, power_im, periodics[note]);

  return power;
}


static void 
scan_freqs() {

  int note, j;
  double power;

  for ( note = lo_note; note<hi_note; note++ ) {

	power = get_power(note);

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

	if ( power > threshold && !act_freq[note] ) 
	  note_on(note, power);

	if (power < (threshold / hysteresis) && act_freq[note] ) 
	  note_off(note, power);
  }
}



static void 
scan_freqs_v2() {

  int note, j;
  short loc_max = 0;
  double power;
  double powers[NTONES];

  powers[lo_note -1] = 0;
  powers[hi_note] = 0;

  for ( note = lo_note; note<hi_note; note++ )
	powers[note] = get_power(note);

  for ( note = lo_note; note<hi_note; note++ ) {

	power = powers[note];

	loc_max = powers[note-1] < power && powers[note+1] < power;

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

	if ( loc_max && power > threshold && !act_freq[note] ) 
	  note_on(note, power);

	if ( power < (threshold / hysteresis) && act_freq[note] ) 
	  note_off(note, power);
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


static void 
midi_write_track_header() {
  assert (midi_file != NULL);

  int chunksize = reorder_dword(60000);

  fwrite("MTrk", 4, 1, midi_file);
  midi_track_chunksize_fixup = ftell(midi_file);
  fwrite(&chunksize, 4, 1, midi_file);
}

static void 
midi_fixup_chunksize() {

  long pos = ftell(midi_file);
  int chunksize = reorder_dword(pos - midi_track_chunksize_fixup - sizeof(dword_t));

  fseek(midi_file,midi_track_chunksize_fixup, SEEK_SET);
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
		  "usage: midimatch OPTIONS\n"
		  "  [-v]             increase verbosity\n"
		  "  [-p]             print frequency table\n"
		  "  [-h]             use harmonic compensation [=%d]\n"
		  "  [-o filename]    write midi data to filename, [=%s]\n"
		  "  [-g gain]        apply gain on input [=%.2f]\n"
		  "  [-N #samples]    input buffer size [=%d]\n"
		  "  [-k hysteresis]  threshold hysteresis [=%.2f]\n"
		  "  [-l]             use local maximum [=%d]\n"
		  "  [-S samplerate]  use samplerate [=%d]\n"
		  "  [-t threshold]   threshold [=%.2f]\n"
		  "  [-r ringsize]    will be used as 2^ringsize [=%d]\n"
		  "PCM input will be read as (44100,8bit,signed,mono) from stdin\n",
		  use_harm_comp,midi_filename, gain, N, 
		  hysteresis, use_local_maximum,SR, threshold, ringsize);
  abort();
}

int 
main(int argc, char** argv) {


  int c, i, rd, j;
  long bytes_read = 0;
  buffer_t* buffer;
  int samples_per_tick;
  div_t rest;

  while ( ( c = getopt(argc, argv, "vphN:o:g:k:r:c:lS:t:") ) != -1) 
	switch (c) {
	case 'v': debug++; break;
	case 'p': print_freqs = 1; break;
	case 'h': use_harm_comp = 1; break;
	case 'N': N = atoi(optarg); break;
	case 'g': gain = atof(optarg); break;
	case 'o': midi_filename =strdup(optarg); break;
	case 'k': hysteresis = atof(optarg); break;
	case 'r': ringsize = atoi(optarg); break;
	case 'l': use_local_maximum = 1; break;
	case 'c': midi_channel = atoi(optarg); break;
	case 'S': SR = atoi(optarg); break;
	case 't': threshold = atof(optarg); break;
	default: usage();
	}

  if ( debug > 0 )
	fprintf(stderr, 
			"ringbuffersize: %d ringbuffermask:0x%08x\n"
			"gain: %f\n",
			RINGBUFFERSIZE, RINGBUFFERMASK, gain);


  print_prologoue(N, SR);


  // allocate buffers
  buffer = (buffer_t*)malloc ( N * sizeof(buffer_t) );
  buffer_f = (double**)malloc ( RINGBUFFERSIZE * sizeof(double*) );
  for ( j = 0; j < RINGBUFFERSIZE; j++ )
	buffer_f[j] = malloc ( N * sizeof(double) );


  // prepare midi file
  midi_file = fopen (midi_filename, "wb");
  midi_write_header();
  midi_write_track_header();


  // prebuffer everthing
  precalculate();

  samples_per_tick = samples_per_midi_tick(SR, midi_bpm, midi_timeDivision);
  rest.rem = 0;

  // process data
  while ( (rd = read(0, buffer, N* sizeof(buffer_t))) ) { 

	bytes_read += rd;

	for ( j = 0; j < N; j++ ) 
	  buffer_f[current_buffer][j] = gain*((double)buffer[j]-128.0);

	rest = div(rest.rem + N, samples_per_tick);
	midi_difftime += rest.quot;

	absolute_time += N;

	if ( use_local_maximum ) scan_freqs_v2();
	else scan_freqs();

	current_buffer = (current_buffer+1) & RINGBUFFERMASK;
  }


  // free stuff
  free(buffer);
  for ( j = 0; j < RINGBUFFERSIZE; j++ ) free(buffer_f[j]);
  free(buffer_f);
  for (i = 0; i< NTONES; i++ ) free(cos_precalc[i]);

  // fixup midi
  midi_write_track_end();
  midi_fixup_chunksize();
  fclose(midi_file);

  // print statistics
  fprintf(stderr, 
		  "\n"
		  "finished.\n"
		  " note_ons:%ld bytes_read:%ld\n", 
		  stats_note_ons, bytes_read);


  print_epilogue();

  return 0;
}
