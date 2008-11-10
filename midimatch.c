#include <rfftw.h>
#include <math.h>

#define N (4096>>1)
#include "common.h"


#define NTONES 128
#define BINLENGTH 256
#define NOSC 30

static float act_freq[NTONES];
static float frequencies[NTONES];
static float frequencies2[NTONES];

static buffer_t buffer[N];




typedef struct tag_oscillator {
  
  byte_t channel;
  byte_t velocity;
  byte_t note;     // == 0xff: osc is free
  float freq;
  unsigned long t;

} oscillator_t;



static byte_t        act_osc[16][128];
static oscillator_t  oscillators[NOSC];
static float threshold = 15;


static float cos_precalc[NTONES][N];

static void calculate_frequencies() {
  int i, n;
  for ( i = 0; i<NTONES; i++ ) {
	frequencies[i] = 6.283185*(440.0/32.0) * pow(2.0, (float)(i-9)/12.0);
	frequencies2[i] = 3.1415 *frequencies[i]/(SRF*4.0);

	for ( n = 0; n < N; n++ ) {
	  
	  cos_precalc[i][n] = cos( (n+0.5)* frequencies2[i])/NF;
	}
  }
}

static void init_osc() {
  int i;
  for (i = 0; i< NOSC; i++) 
	oscillators[i].note = 0xff;
}

static inline byte_t process_osc(oscillator_t* osc) {
  float f = 0.2*0.5*
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


  int osc;
  int i, n;
  float power;
  
  for ( i = 36; i<NTONES-10; i++ ) {
		
	// dctII:
	power = 0.0f;
	for ( n = 0; n < N; n++ ) 
	  power += buffer[n] * cos_precalc[i][n];

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
	}

	if (power < threshold * 0.1 && act_freq[i] ) {
	  act_freq[i] = 0;
	  oscillators[act_osc[0][i]].note = 0xff;
	}

  }

}

int main(int argc, char** argv) {

  calculate_frequencies();
  init_osc();

  int i, rd, j;
  long bytes = 0;

  if (argc != 2) 
	die ("usage: midimatch threshold");

  threshold = atoi (argv[1]);

  fprintf(stderr, "starting (midimatch)\n"); 



  while ( 1 ) { //&& bytes < 500000ul ) {
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

	rd = write(1, buffer, rd);
	bytes += rd;

  }

  return 0;

}
