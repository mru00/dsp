#include <rfftw.h>
#include <math.h>

#define N (4096>>2)
#include "common.h"


#define NTONES 128
#define BINLENGTH 256
#define NOSC 30

float act_freq[NTONES];
float frequencies[NTONES];
buffer_t buffer[N];




typedef struct tag_oscillator {
  
  byte_t channel;
  byte_t velocity;
  byte_t note;     // == 0xff: osc is free
  float freq;
  unsigned long t;

} oscillator_t;

byte_t        act_osc[16][128];

oscillator_t  oscillators[NOSC];


static void calculate_frequencies() {
  int i;
  for ( i = 0; i<NTONES; i++ ) {
	frequencies[i] = 6.283185*(440.0/32.0) * pow(2.0, (float)(i-9)/12.0);
  }
}

static void init_osc() {
  int i;
  for (i = 0; i< NOSC; i++) 
	oscillators[i].note = 0xff;
}

static byte_t process_osc(oscillator_t* osc) {
  float f = 0.2*0.6*
	(float)osc->velocity * 
	sinf(osc->freq * (osc->t++));

  return f;
}

static int get_free_osc() {
  int i;
  for (i = 0; i< NOSC; i++) 
	if (oscillators[i].note == 0xff) return i;

  fprintf(stderr, "not enouth free oscillators\n");
  return -1;
}

static void printnotes() {

  const float threshold = NF*40;

  int osc;
  int i, n;
  float power;
  
  for ( i = 20; i<NTONES-20; i++ ) {
	
	
	// dct:
	power = 0;
	for ( n = 0; n < N; n++ ) {
	  power += buffer[n] * cos( 3.1415 * (n+0.5)* frequencies[i]/(SRF*4.0));
	}

	power = abs(power);

	if ( power > threshold && !act_freq[i] ) {

	  act_freq[i] = 1;

	  osc = get_free_osc();
	  if (osc != -1) {
		oscillators[osc].t = 0;
		oscillators[osc].note = i;
		oscillators[osc].velocity = 127;
		oscillators[osc].freq = frequencies[i-4]/SRF;
		act_osc[0][i] = osc;
	  }
	  
	  fprintf(stderr, "note on:  i:%d power:%f, freq:%f\n", i, log(power)/log(2), frequencies[i]);
	}

	if (power < threshold * 0.05 && act_freq[i] ) {
	  act_freq[i] = 0;
	  fprintf(stderr, "note off: i:%d power:%f\n", i, log(power)/log(2));
	  oscillators[act_osc[0][i]].note = 0xff;
	}
  }

}

int main(int argc, char** argv) {

  calculate_frequencies();
  init_osc();

  int i, rd, j;
  long bytes = 0;

  while ( 1 ) {
	// read sample
	rd = read(0, buffer, N* sizeof(buffer_t)); 

	// eof
	if ( rd != N*sizeof(buffer_t) )  {
	  fprintf(stderr, "%li bytes processed, %d read, exiting (midimatch)\n", 
			  bytes, rd);
	  return 0;
	}

	printnotes();

	for ( j = 0; j < N*sizeof(buffer_t); j++ ) {

	  buffer[j] = 128;
	  for ( i = 0; i< NOSC; i++)
		if (oscillators[i].note != 0xff)
		  buffer[j] += process_osc(oscillators+i);
	  
	}

	fprintf(stderr, ".");
	rd = write(1, buffer, rd);
	bytes += rd;

  }

  return 0;

}
