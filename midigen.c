#include <stdlib.h>
#include <math.h>

#define N (4096<<0)


#include "common.h"


#define NOSC 10
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
  byte_t note;     // == 0xff: osc is free
  float freq;
  unsigned long t;

} oscillator_t;


static oscillator_t  oscillators[NOSC];
static float         frequencies[128];
static float         channel_volume[16];



static void calculate_frequencies() {
  int i;
  for ( i = 0; i<128; i++ ) {
	frequencies[i] = 2.0*6.283185*(440.0/32.0) * pow(2.0, (float)(i-9)/12.0) / SRF;
  }
}


static void init_osc() {
  int i;
  for (i = 0; i< NOSC; i++) 
	oscillators[i].note = 0xff;
}

static byte_t process_osc(oscillator_t* osc) {
  float f = 0.2*
	channel_volume[osc->channel] *
	(float)osc->velocity * 
	sinf(osc->freq * (osc->t++));

  return f;
}

int main(int argc, char** argv) {

  byte_t output;
  int j;
  int note;

  if ( argc != 2 )
	die ("usage: midigen midinote");

  note = atoi ( argv[1]);

  calculate_frequencies();
  init_osc();

  for ( j = 0; j< 16; j++)
	channel_volume[j] = 0.7;

  oscillators[0].t = 0;
  oscillators[0].note = note;
  oscillators[0].velocity = 127;
  oscillators[0].channel = 0;
  oscillators[0].freq = frequencies[oscillators[0].note];

  while ( 1 ) {

	// calculate output
	output = 128;

	output += process_osc(oscillators);

	// write output
	fwrite(&output, 1, 1, stdout);
  }

  return 0;
}
