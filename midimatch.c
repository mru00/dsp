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
#include "midifile.h"
#include "input.h"
#include <alsa/asoundlib.h>

// number of midi notes 
#define NTONES 128     

#define RINGBUFFERSIZE (1<<ringsize)
#define RINGBUFFERMASK (RINGBUFFERSIZE-1)

typedef float real_t;

// buffers & statistics
static int           periodics[NTONES];
static real_t        *cos_precalc[NTONES];
static real_t        act_freq[NTONES];
static real_t        **buffer_f;
static real_t        threshold = 35.0;
static real_t        weighting_ELC[NTONES];
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
static real_t        gain = 1.0;
static real_t        hysteresis = 7;
static short         ringsize = 4;
static short         current_buffer=0;
static short         use_local_maximum = 0;

// midi config
static midi_file_t*  midi_file = NULL;
static short         midi_channel = 2;
static const real_t  midi_bpm = 120.0;
static const int     midi_timeDivision = 120;
static char*         midi_filename = NULL;
static snd_seq_t*    midi_sequencer = NULL;


// http://en.wikipedia.org/wiki/A-weighting
static real_t 
ra2(real_t f) {

  const real_t a2 = 12200*12200;
  const real_t b2 = 20.6*20.6;
  const real_t c2 = 107.7*107.7;
  const real_t d2 = 737.9*737.9;
  real_t f2 = f*f;

  real_t ra = a2 * f2*f2 / ( (f2+b2) * sqrt( (f2+c2)*(f2+d2) ) * (f2+a2) ); 
  return ra;
  return 2.0 + 20*log10(ra);
}

static void 
precalculate() {

  real_t rad_per_sample;
  int note, k, per, p;
  for ( note = lo_note; note< hi_note; note++ ) {

	rad_per_sample = midi_note_to_radians_per_sample(SR,note);

	per = ceil( 2.0 * pi/rad_per_sample);

	cos_precalc[note] = (real_t*)malloc ( 2* per * sizeof(real_t));

	for ( p = 0, k = 0; k < per; k++ ) {
	  cos_precalc[note][p++] = cos( rad_per_sample * (real_t) k );
	  cos_precalc[note][p++] = sin( rad_per_sample * (real_t) k );
	}

	weighting_ELC[note] = ra2(midi_note_to_hertz(note));

	periodics[note] = per;

	if (print_freqs) 
	  fprintf(stderr, 
			  "note: %-3d weighting:%-5.1f periodics:%i\n", 
			  note,  weighting_ELC[note], per);
  }
}

static void
midi_connect_sequencer() {
  int err;
  err = snd_seq_open(&midi_sequencer, "default", SND_SEQ_OPEN_OUTPUT, 0);
  assert (err >= 0);
  snd_seq_set_client_name(midi_sequencer, "midimatch");

  snd_seq_create_simple_port(midi_sequencer, "matched midi data",
							 SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
							 SND_SEQ_PORT_TYPE_MIDI_GENERIC);

  snd_seq_connect_to(midi_sequencer, 0, 128, 0);
}

static void 
note_on(int note, real_t power) {

  snd_seq_event_t ev;

  if (debug>0) fprintf(stderr, ".");
  if (debug>1) 
	fprintf(stderr, "midimatch: (%ld) note on:  note:%-3d p:%.2f\n", 
			absolute_time, note, power);

  
  act_freq[note] = 1;
  if (midi_file != NULL )
	midi_write_note_event(0x90 | midi_channel, note, 100, midi_file);
  stats_note_ons ++;

  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, 0);
  snd_seq_ev_set_subs(&ev);
  snd_seq_ev_set_direct(&ev);

  snd_seq_ev_set_noteon(&ev, midi_channel, note, 100) ;
  
  snd_seq_event_output(midi_sequencer, &ev);
  snd_seq_drain_output(midi_sequencer);
	
}


static void
note_off(int note, real_t power) {

  snd_seq_event_t ev;

  if (debug>1) 
	fprintf(stderr, "midimatch: (%ld) note off: note:%-3d p:%.2f\n", 
			absolute_time, note, power);
  

  act_freq[note] = 0;
  if (midi_file != NULL )
	midi_write_note_event(0x80 | midi_channel, note, 100, midi_file);


  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, 0);
  snd_seq_ev_set_subs(&ev);
  snd_seq_ev_set_direct(&ev);

  snd_seq_ev_set_noteoff(&ev, midi_channel, note, 100) ;
  
  snd_seq_event_output(midi_sequencer, &ev);
  snd_seq_drain_output(midi_sequencer);
}


static real_t
get_power(int note) {
  real_t power_re = 0.0f;
  real_t power_im = 0.0f;
  real_t power = 0.0f;

  int p, t, j, k;
  int periodic = 2 * periodics[note];
  real_t* bufferp;
  real_t xt =0.0f;

  // p: k modulo (periodicity of sin, cos)
  // t: true index variable over all buffers
  // k: index variable over one buffer

  t = p = 0;
  for ( j = 0; j < RINGBUFFERSIZE; j++ ) {
	bufferp = buffer_f[(current_buffer + j + 1) & RINGBUFFERMASK];
	
	// z-transform
	for ( k = 0; k < N ; k++ ) {

	  xt = bufferp[k] * ( 0.54 - 0.46 * cos( 2* pi * t / ((RINGBUFFERSIZE*N)-1) ) );
	  t++;

 	  power_re += xt * cos_precalc[note][p++]; 
 	  power_im += xt * cos_precalc[note][p++]; 

	  if ( p == periodic ) p = 0;
	}
  }

  // power = 20 log10 ( |power| )
  power = 1000*absolute(power_re,power_im);
  power *= weighting_ELC[note];
  power /= N*RINGBUFFERSIZE;
  power /= gain;


  if (debug>1) 
	fprintf(stderr, "midimatch: (%ld) note:%-3d power:%-5.2f "
			"weighting:%-5.2f re:%-5.2f im:%-5.2f periodics:%d\n", 
			absolute_time, note, power, weighting_ELC[note], 
			power_re, power_im, periodics[note]);

  return power;
}


static void 
scan_freqs() {

  int note, j;
  real_t power;

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
  real_t power;
  real_t powers[NTONES];

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
		  "PCM input will be read as (%d-byte floating point,mono) from stdin\n",
		  use_harm_comp,midi_filename, gain, N, 
		  hysteresis, use_local_maximum,SR, threshold, ringsize, sizeof(real_t));
  abort();
}

int 
main(int argc, char** argv) {


  int c, i, rd, j;
  long bytes_read = 0;
  int samples_per_tick;
  div_t rest;
  buffer_t* buffer;

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

  buffer = input_open("stdin", N);

  // allocate buffers

  buffer_f = (real_t**)malloc ( RINGBUFFERSIZE * sizeof(real_t*) );
  for ( j = 0; j < RINGBUFFERSIZE; j++ )
	buffer_f[j] = malloc ( N * sizeof(real_t) );


  // prepare midi file
  if (midi_filename != NULL ) {
	midi_file = midi_write_open(midi_filename);
	midi_write_header(midi_timeDivision, 1, midi_file);
	midi_write_track_header(midi_file);
  }

  midi_connect_sequencer();


  // prebuffer everthing
  precalculate();

  samples_per_tick = samples_per_midi_tick(SR, midi_bpm, midi_timeDivision);
  rest.rem = 0;

  // process data
  while ( (rd = read(0, buffer_f[current_buffer], N*sizeof(real_t))) ) { 

	bytes_read += rd;

	rest = div(rest.rem + N, samples_per_tick);

	if (midi_file != NULL )
	  midi_write_increase_difftime(rest.quot, midi_file);


	absolute_time += N;

	if ( use_local_maximum ) scan_freqs_v2();
	else scan_freqs();

	current_buffer = (current_buffer+1) & RINGBUFFERMASK;
  }



  // free stuff
  for ( j = 0; j < RINGBUFFERSIZE; j++ ) free(buffer_f[j]);
  free(buffer_f);
  for (i = 0; i< NTONES; i++ ) free(cos_precalc[i]);

  // fixup midi
  if (midi_file != NULL ) {
	midi_write_track_end(midi_file);
	midi_write_close(midi_file);
  }

  // print statistics
  fprintf(stderr, 
		  "\n"
		  "finished.\n"
		  " note_ons:%ld bytes_read:%ld playtime:%ld:%ld\n", 
		  stats_note_ons, bytes_read, bytes_read/(SR*60), (bytes_read / SR)%60 );

  input_close();

  print_epilogue();

  return 0;
}
