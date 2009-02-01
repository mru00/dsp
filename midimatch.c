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


#define max(a,b) ( (a)>(b) ? (a) : (b))


// number of midi notes 
#define NTONES 128     

#define RINGBUFFERSIZE (1<<ringsize)
#define RINGBUFFERMASK (RINGBUFFERSIZE-1)

typedef float real_t;

// buffers & statistics
static real_t        max_powers[NTONES];
static real_t        *cos_precalc[NTONES];
static real_t        act_freq[NTONES];
static real_t        **buffer_f;
static real_t        threshold = 15.0;
static real_t        weighting_ELC[NTONES];
static unsigned long absolute_time = 0;
static unsigned long stats_note_ons = 0;

// config with default values
static int           N = 512;
static int           SR = 44100;
static short         debug = 0;
static short         print_freqs = 0;
static short         print_statistics = 0;
static short         use_harm_comp = 0;
static short         use_sequencer = 0;
static real_t        gain = 1.0;
static real_t        hysteresis = 7;
static short         ringsize = 4;
static const int     lo_note = 30;
static const int     hi_note = 74;

static short         current_buffer=0;


// midi config
static midi_file_t*  midi_file = NULL;
static short         midi_channel = 2;
static const real_t  midi_bpm = 120.0;
static const int     midi_timeDivision = 120;
static char*         midi_filename = NULL;
static snd_seq_t*    midi_sequencer = NULL;
static const char*   midi_notenames[] =
  { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };



// the name of the note, human readable
// http://tonalsoft.com/pub/news/pitch-bend.aspx
static const char* midi_notename(int note) {
  return midi_notenames[note%12];
}

static int midi_octave(int note) {
  return note/12 - 5;
}

// http://en.wikipedia.org/wiki/A-weighting
static real_t 
ra2(real_t f) {

  return 1.0;

  const real_t a2 = 12200*12200;
  const real_t b2 = 20.6*20.6;
  const real_t c2 = 107.7*107.7;
  const real_t d2 = 737.9*737.9;
  real_t f2 = f*f;

  real_t ra = a2 * f2*f2 / ( (f2+b2) * sqrt( (f2+c2)*(f2+d2) ) * (f2+a2) ); 
  //  return ra;
  return 2.0 + 20*log10(ra);
}

static inline real_t 
window(unsigned int n, unsigned int N) {


#if 0
  // gauss window
  float sigma = 0.4;
  float e = (n-(N-1)/2 ) / ( sigma* (N-1) /2);
  return exp ( -.05 * e * e );

#elif 1

  // barlett-hann
  float a0 = 0.62, a1 = 0.48, a2 = 0.38;
  return a0 - a1*abs( n/(N-1) - 0.5) - a2*cos(2*pi*n/(N-1));

#elif 0

  // hann window
  return 0.5 * ( 1- cos( 2*pi*n/(N-1) ) );

#elif 0

  // hamming window
  return ( 0.54 - 0.46 * cos( 2* pi * n / ((N)-1) ) );

#endif
}

static void 
precalculate() {

  real_t rad_per_sample;
  int note, k, p;
  for ( note = lo_note; note< hi_note; note++ ) {

	max_powers[note] = 0.0f;

	rad_per_sample = midi_note_to_radians_per_sample(SR, note);

	cos_precalc[note] = (real_t*)malloc ( 2*RINGBUFFERSIZE*N*sizeof(real_t));

	for ( p = 0, k = 0; k < (N*RINGBUFFERSIZE); k++ ) {
	  float win = window( k, RINGBUFFERSIZE * N);
	  cos_precalc[note][p++] = cos( rad_per_sample * (real_t) k ) * win;
	  cos_precalc[note][p++] = sin( rad_per_sample * (real_t) k ) * win;
	}

	weighting_ELC[note] = ra2(midi_note_to_hertz(note));

	if (print_freqs) 
	  fprintf(stderr, 
			  "note: %-3d weighting:%-5.1f freq:%.2f\n", 
			  note,  weighting_ELC[note], midi_note_to_hertz(note));
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

  if (debug>1) 
	fprintf(stderr, "midimatch: (%ld) note on:  note:%-3d p:%.2f name:%s@%d\n", 
			absolute_time, note, power, 
			midi_notename(note), midi_octave(note));
  else if (debug>0) 
	fprintf(stderr, ".");
 
  stats_note_ons ++;
 
  act_freq[note] = 1;
  if (midi_file != NULL ) {
	midi_write_note_event(0x90 | midi_channel, note, 100, midi_file);
  }

  if ( use_sequencer ) {
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, 0);
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);
  
	snd_seq_ev_set_noteon(&ev, midi_channel, note, 100) ;
	
	snd_seq_event_output(midi_sequencer, &ev);
	snd_seq_drain_output(midi_sequencer);
  }
}


static void
note_off(int note, real_t power) {

  snd_seq_event_t ev;

  if (debug>2) 
	fprintf(stderr, "midimatch: (%ld) note off: note:%-3d p:%.2f maxp:%0.2f\n", 
			absolute_time, note, power, act_freq[note]);
  

  act_freq[note] = 0.0f;
  if (midi_file != NULL )
	midi_write_note_event(0x80 | midi_channel, note, 100, midi_file);

  if ( use_sequencer ) {
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, 0);
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);

	snd_seq_ev_set_noteoff(&ev, midi_channel, note, 100) ;
	
	snd_seq_event_output(midi_sequencer, &ev);
	snd_seq_drain_output(midi_sequencer);
  }
}


static real_t
get_power(int note) {
  real_t power_re = 0.0f;
  real_t power_im = 0.0f;
  real_t power = 0.0f;

  int t, j, k;
  real_t* bufferp;
  real_t xt = 0.0f;

  // p: k modulo (periodicity of sin, cos)
  // t: true index variable over all buffers
  // k: index variable over one buffer

  t = 0;
  for ( j = 0; j < RINGBUFFERSIZE; j++ ) {
	bufferp = buffer_f[(current_buffer + j + 1) & RINGBUFFERMASK];
	
	// z-transform
	for ( k = 0; k < N ; k++ ) {
	  xt = bufferp[k];

 	  power_re += xt * cos_precalc[note][t++]; 
 	  power_im += xt * cos_precalc[note][t++]; 
	}
  }

  // power = 20 log10 ( |power| )
  power = 1000*absolute(power_re,power_im);
  power /= N*RINGBUFFERSIZE;


  max_powers[note] = max( max_powers[note], power);

  power *= weighting_ELC[note];
  power *= gain;


  if (debug>2) 
	fprintf(stderr, "midimatch: (%ld) note:%-3d power:%-5.2f "
			"weighting:%-5.2f re:%-5.2f im:%-5.2f\n", 
			absolute_time, note, power, weighting_ELC[note], 
			power_re, power_im);

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

	if ( act_freq[note] ) act_freq[note] =max(act_freq[note], power);

	if ( power > threshold && !act_freq[note] ) 
	  note_on(note, power);

	if (power < (threshold / hysteresis) && act_freq[note] ) 
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
		  "  [-o filename]    write midi data to filename (seq: use alsa sequencer)\n"
		  "  [-g gain]        apply gain on input [=%.2f]\n"
		  "  [-N #samples]    input buffer size [=%d]\n"
		  "  [-k hysteresis]  threshold hysteresis [=%.2f]\n"
		  "  [-S samplerate]  use samplerate [=%d]\n"
		  "  [-t threshold]   threshold [=%.2f]\n"
		  "  [-i input]       input selection { stdin, alsa:devicename }\n"
		  "  [-s]             print statistics\n"
		  "  [-r ringsize]    will be used as 2^ringsize [=%d]\n"
		  "PCM input will be read as (%d-byte floating point,mono) from stdin\n",
		  use_harm_comp, gain, N, 
		  hysteresis, SR, threshold, ringsize, sizeof(real_t));
  abort();
}

int 
main(int argc, char** argv) {


  int c, i, rd, j;
  long bytes_read = 0;
  int samples_per_tick;
  div_t rest;
  rdfun input_read;
  char* input = "alsa:default";

  while ( ( c = getopt(argc, argv, "vphN:o:g:k:r:c:lS:i:t:s") ) != -1) 
	switch (c) {
	case 'v': debug++; break;
	case 'p': print_freqs = 1; break;
	case 'h': use_harm_comp = 1; break;
	case 'N': N = atoi(optarg); break;
	case 'g': gain = atof(optarg); break;
	case 'o': 
	  if (strcmp(optarg, "seq") == 0 ) use_sequencer = 1;
	  else midi_filename =strdup(optarg); 
	  break;
	case 'k': hysteresis = atof(optarg); break;
	case 'r': ringsize = atoi(optarg); break;
	case 'c': midi_channel = atoi(optarg); break;
	case 'i': input = strdup(optarg); break;
	case 'S': SR = atoi(optarg); break;
	case 's': print_statistics = 1; break;
	case 't': threshold = atof(optarg); break;
	default: usage();
	}

  if ( debug > 0 )
	fprintf(stderr, 
			"ringbuffersize: %d ringbuffermask:0x%08x\n"
			"gain: %f\n",
			RINGBUFFERSIZE, RINGBUFFERMASK, gain);



  print_prologoue(N, SR);

  input_read = input_open(input);

  // allocate buffers

  buffer_f = (real_t**)malloc ( RINGBUFFERSIZE * sizeof(real_t*) );
  for ( j = 0; j < RINGBUFFERSIZE; j++ ) {
	buffer_f[j] = malloc ( N * sizeof(real_t) );
	for ( i = 0; i < N ; i ++ )
	  buffer_f[j][i] = 0.0;
  }


  // prepare midi file
  if (midi_filename != NULL ) {
	midi_file = midi_write_open(midi_filename);
	midi_write_header(midi_timeDivision, 1, midi_file);
	midi_write_track_header(midi_file);
  }

  if ( use_sequencer )
	midi_connect_sequencer();


  // prebuffer everthing
  precalculate();

  samples_per_tick = samples_per_midi_tick(SR, midi_bpm, midi_timeDivision);
  rest.rem = 0;

  // process data
  while ( (rd = input_read(buffer_f[current_buffer], N)) ) { 

	bytes_read += rd;

	rest = div(rest.rem + N, samples_per_tick);

	if (midi_file != NULL )
	  midi_write_increase_difftime(rest.quot, midi_file);

	absolute_time += N;

 	scan_freqs(); 

	// advance buffer:
	current_buffer = (current_buffer+1) & RINGBUFFERMASK;
  }



  for ( j = lo_note; j < hi_note; j++ ) {
	if ( act_freq[j] )
	  note_off(j, 0);
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


  if ( print_statistics ) {

	// octave style output of powers over frequency
	// first row: frequencies
	// second row: power for that freq
	fprintf(stderr, "freqs = [");
	for (j=lo_note;j<hi_note-1;j++) fprintf(stderr, "%.2f,", midi_note_to_hertz(j));
	fprintf(stderr, "%.2f", midi_note_to_hertz(j));
	fprintf(stderr, ";");
	for (j=lo_note;j<hi_note-1;j++) fprintf(stderr, "%.2f,", max_powers[j]);
	fprintf(stderr, "%.2f", max_powers[j]);
	fprintf(stderr, "];\n");
	
	fprintf(stderr, 
			"\n\n note_ons:%ld bytes_read:%ld playtime:%ld:%ld\n", 
			stats_note_ons, bytes_read, bytes_read/(SR*60), (bytes_read / SR)%60 );
  }


  input_close();

  print_epilogue();

  return 0;
}
