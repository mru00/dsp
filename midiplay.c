#include <getopt.h>
#include <stdlib.h>
#include <math.h>

#include "common.h"

const int N=4096;
const int SR=44100;

#define NOSC 20
#define NCH 16
#define NNOTE 128

/*
 *   0x00	4	char[4]	chunk ID	"MThd" (0x4D546864)
 *   0x04	4	dword	chunk size	6 (0x00000006)
 *   0x08	2	word	format type	0 - 2
 *   0x10	2	word	number of tracks	1 - 65,535
 *   0x12	2	word	time division	see following text
 */
typedef struct tag_midi_header {
  byte_t  chunkID[4];
  dword_t chunkSize;
  word_t  formatType;
  word_t  numberOfTracks;
  word_t  timeDivision;
} midi_header_t;


/*
 *   0x00	4	char[4]	chunk ID	"MTrk" (0x4D54726B)
 *   0x04	4	dword	chunk size	see following text
 *   0x08	track event data (see following text)
 */
typedef struct tag_track_header {
  byte_t  chunkID[4];
  dword_t chunkSize;

  byte_t* eventData;
  byte_t* nextEvent;

  unsigned long decrementedDiffTime;
  int     trackID;
  byte_t  done;

  byte_t status;
} track_header_t;


typedef struct tag_oscillator {
  
  byte_t channel;
  byte_t velocity;
  //  byte_t note;     // == 0xff: osc is free
  float freq;
  unsigned long t;

} oscillator_t;


static oscillator_t  oscillators[NOSC];
static byte_t        act_osc[16][128];
static float         frequencies[128];
static float         channel_volume[16];
static FILE*         midi_file = NULL;
static int           bpm = 120;
static int           samples_per_tick;
static midi_header_t midi_header;
static long          absolute_time = 0;
static unsigned int selected_channels = 0;
static unsigned int selected_tracks = 0;


static byte_t* read_var_length(byte_t* ptr, unsigned long* result);
static int get_free_osc();

static short debug = 0;
static short print_notes = 0;



// parse midi header
static void read_midi_header() {

  assert (midi_file != NULL);

  fread(&midi_header.chunkID, 4, 1, midi_file);
  fread(&midi_header.chunkSize, 4, 1, midi_file);
  fread(&midi_header.formatType, 2, 1, midi_file);
  fread(&midi_header.numberOfTracks, 2, 1, midi_file);
  fread(&midi_header.timeDivision, 2, 1, midi_file);

  // adapt byte-order
  midi_header.formatType     = reorder_word(midi_header.formatType);
  midi_header.numberOfTracks = reorder_word(midi_header.numberOfTracks);
  midi_header.timeDivision   = reorder_word(midi_header.timeDivision);
  midi_header.chunkSize      = reorder_dword(midi_header.chunkSize);
}


// parse track header
static void read_track_header(track_header_t* hdr) {
  assert (midi_file != NULL);
  assert (hdr != NULL);

  fread(&hdr->chunkID, 4, 1, midi_file);
  fread(&hdr->chunkSize, 4, 1, midi_file);

  hdr->chunkSize = reorder_dword(hdr->chunkSize);
  hdr->eventData = (byte_t*) malloc(hdr->chunkSize);
  hdr->nextEvent = hdr->eventData;
  hdr->done = 0;

  
  if ( debug > 0 ) fprintf(stderr, "reading track chucksize:%d\n", hdr->chunkSize);

  fread(hdr->eventData, hdr->chunkSize, 1, midi_file);

  // preload decrementedDiffTime
  read_var_length(hdr->eventData, &hdr->decrementedDiffTime);
}


// returns the number of bytes read
// *result will get length
static byte_t* read_var_length(byte_t* ptr, unsigned long* result) {

  unsigned long tmp = 0;
  byte_t b;

  do {
	b = *(ptr++);
	tmp = (tmp << 7) + ( b & 0x7f );
  } while ( b & 0x80 );

  *result = tmp;
  return ptr;
}


// returns the length at *evt
// does not modify *evt
static unsigned long get_difftime(byte_t* evt) {
  unsigned long difftime;
  read_var_length(evt, &difftime);
  return difftime;
}

// returns the pointer to the next event
static byte_t* process_event(track_header_t* hdr, byte_t* ptr) {

  unsigned long difftime;
  unsigned long length;
  byte_t eventType;
  byte_t channel;
  byte_t par1=0, par2=0;
  unsigned long tempo;
  int osc;

  if ( debug > 1 ) fprintf (stderr, "process event at %p\n", ptr);

  // difftime
  ptr = read_var_length(ptr, &difftime);

  // running status handling
  if ( (*ptr & 0x80) == 0x80 )
	hdr->status = *(ptr++);

  // meta event
  if ( hdr->status == 0xff ) {

	// type
	eventType = *(ptr++);

	// length
	ptr = read_var_length(ptr, &length);

	switch (eventType) {
	case 0x2F: // end of track
	  hdr->done = 1;
	  break;
	case 0x51: // set tempo
	  assert (length == 3);
	  tempo = (((ptr[0]<<16)|(ptr[1]<<8)|(ptr[2]))); 
	  bpm = 60000000.0/ (float)tempo;
	  samples_per_tick = samples_per_midi_tick(SR,bpm,midi_header.timeDivision);
	  fprintf(stderr, 
			  "new bpm: %d, par1:%d samples_per_tick:%d div:%d length:%d\n", 
			  bpm, tempo, samples_per_tick, length, midi_header.timeDivision);
	  break;
	} 

	// skip meta data
	ptr += length;
  }
  // sysex event
  else if ( hdr->status == 0xF0  || hdr->status == 0xF7 ) {

	// length
	ptr = read_var_length(ptr, &length);
	// skip sysex data
	ptr += length;
  }
  // "normal" midi event
  else{

	// parse type
	eventType = (hdr->status >> 4) & 0xf;
	channel = hdr->status & 0xf;

	par1 = *(ptr++);

	// some events have only one parameter
	if ( eventType != 0xC && eventType != 0xD )
	  par2 = *(ptr++);	  
	
	switch ( eventType ) {
	case 0x8: // note off
	  oscillators[act_osc[channel][par1]].velocity = 0;
	  if ( print_notes ) fprintf (stderr, 
								  "midiplay: (%ld) note off ch:%-2d i:%-3d\n", 
								  absolute_time, channel, par1);
	  break;
 	case 0x9: // note on

	  // channel globally selected?
	  if ( ( (1 << channel) & selected_channels ) == 0 ) break;

	  if ( par2 != 0 ) {
		osc = get_free_osc();
		if (osc != -1) {
		  oscillators[osc].t = 0;
		  oscillators[osc].velocity = par2;
		  oscillators[osc].channel = channel;
		  oscillators[osc].freq = frequencies[par1];
		  act_osc[channel][par1] = osc;
		}

		if ( print_notes ) fprintf (stderr, 
									"midiplay: (%ld) note on  ch:%-2d i:%-3d\n", 
									absolute_time, channel, par1);

	  }
	  else {
		oscillators[act_osc[channel][par1]].velocity = 0;
	  }
	  break;
	case 0xD: // channel aftertouch
	  switch(par1) {
	  case 0x07: 
		channel_volume[channel] = (float)par2/127.0; 
		break;
	  }
	  break;
	case 0xA: // note aftertouch
	case 0xB: // controller
	case 0xC: // program change
	case 0xE: // pitch bend
	  
	  break;
	default: 
	  die ("invalid midi event");
	}
  }

  // track fully processed?
  if ( ptr == (hdr->eventData + hdr->chunkSize ) )
	hdr->done = 1;

  return ptr;
}


// processes all events for the given track
static void process_next_event_for_track(track_header_t* hdr) {

  byte_t* ptr = hdr->nextEvent;

  if ( hdr->decrementedDiffTime == 0 ) {

	ptr = process_event(hdr, ptr);

	// more events with diff-time = 0?
	while ( get_difftime(ptr) == 0 && !hdr->done )
	  ptr = process_event(hdr, ptr);

	hdr->decrementedDiffTime = get_difftime(ptr);
	hdr->nextEvent = ptr;
  }
  else {
	hdr->decrementedDiffTime --;
  }
}

static void calculate_frequencies() {
  int i;
  for ( i = 0; i<128; i++ ) {
	frequencies[i] = midi_note_to_radians_per_sample(SR, i);
  }
}


static void init_osc() {
  int i;
  for (i = 0; i< NOSC; i++) 
	oscillators[i].velocity = 0;
}

static byte_t process_osc(oscillator_t* osc) {
  float f = 0.2*
	channel_volume[osc->channel] *
	(float)osc->velocity * 
	sinf(osc->freq * osc->t);

  osc->t++;
  return f;
}

static int get_free_osc() {
  int i;
  for (i = 0; i< NOSC; i++) 
	if (oscillators[i].velocity == 0) return i;

  fprintf(stderr, "not enouth free oscillators\n");
  return -1;
}

static void usage() {
  fprintf(stderr, 
		  "usage: midiplay OPTIONS midifile\n"
		  "[-p]    print notes\n"
		  "[-d]    increase debug level\n"
		  "{-c channel} process channels only 1..\n"
		  "{-t track} process tracks only 1..\n");
  abort();
}

int main(int argc, char** argv) {

  track_header_t* track_headers;
  int sample_count = 0;
  byte_t output;
  int i, j;
  int done;
  int c;

  while ( (c=getopt(argc, argv, "dpc:t:")) != -1) 
	switch(c) {
	case 'd': debug ++; break;
	case 'p': print_notes = 1; break;
	case 'c': selected_channels |= 1<< (atoi(optarg)-1); break;
	case 't': selected_tracks |= 1<< (atoi(optarg)-1); break;
	default: usage();
	}

  if ( selected_channels == 0 ) 
	selected_channels = 0xffffffff;

  if ( selected_tracks == 0 ) 
	selected_tracks = 0xffffffff;
  
  if ( debug > 0 ) 
	fprintf(stderr, "selected channels: 0x%04x tracks:0x%08x\n", 
			selected_channels, selected_tracks);

  if (argc == optind)
	die("usage: midi midifile");

  if ( strncmp ( argv[optind], "-", 1) == 0 )
	midi_file = stdin;
  else 
	midi_file = fopen(argv[optind++], "rb");

  optind++;

  if (midi_file == NULL)
	die("failed to open midifile");

  print_prologoue(N, SR);

  read_midi_header(&midi_header);

  track_headers = (track_header_t*) 
	malloc(midi_header.numberOfTracks*sizeof(track_header_t));

  for (i = 0; i< midi_header.numberOfTracks; i++ ) {
	read_track_header(track_headers + i);
	track_headers[i].trackID = i;
  }

  fclose(midi_file);
  midi_file = NULL;

  samples_per_tick = samples_per_midi_tick(SR,bpm,midi_header.timeDivision);  
  calculate_frequencies();
  init_osc();

  for ( j = 0; j< 16; j++)
	channel_volume[j] = 0.7;

  while ( 1 ) {
	
	absolute_time ++;

	// midi events ready?
	if (sample_count == samples_per_tick) {

	  if ( debug > 2 ) 
		fprintf(stderr, "processing next sample samples_per_tick=%d\n", 
				samples_per_tick);

	  sample_count = 0;

	  done = 0;
	  for (i = 0; i< midi_header.numberOfTracks; i++ ) {
		if ( !track_headers[i].done && (1<<i) & selected_tracks) {
		  
		  if ( debug > 3 ) fprintf(stderr, "(%ld) processing track:%d\n", 
								   absolute_time, i);

		  process_next_event_for_track(track_headers + i );
		  done ++;
		}
	  }

	  // no more active tracks...
	  if ( done == 0 ) break;
	}

	sample_count ++;

	// calculate output
	output = 128;

	for ( i = 0; i< NOSC; i++)
	  if (oscillators[i].velocity != 0)
		output += process_osc(oscillators+i);

	// write output
	fwrite(&output, 1, 1, stdout);
  }

  print_epilogue();

  return 0;
}
