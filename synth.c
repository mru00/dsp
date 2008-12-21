/*
 * synthesizes a note using calculated ck-values
 *
 * base frequency = F_BASE
 *
 * make && ./synth | play -s -1 -c 1 -t raw -r 44100 -
 *
 * output: written to stdout
 */

#include <rfftw.h>

#include "common.h"

const int N = 4096;
const int SR = 44100;

#define F_BASE 30

int main(void) {

  fftw_real out[N], in[N];
  rfftw_plan plan_backward;
  buffer_t buffer[N];

  int i, rd;
  long bytes = 0;

  plan_backward = rfftw_create_plan(N, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);

  print_prologoue(N, SR);

  while ( 1 ) {

	out[0] = 0.0;
	for ( i = 1; i < N/2; i++ ) {
	  if ( i % F_BASE == 0 ){
		out[i] = out[N-i] = N * exp(-10*i/N) / 5.0;
	  }
	  else {
		out[i] = out[N-i] = 0.0f;
	  }
	}

	
	rfftw_one(plan_backward, out, in);

	for ( i = 0; i < N; i++ ) 
	  buffer[i] = in[i]/N;

  	rd = write(1, buffer, N* sizeof(buffer_t));
	bytes += rd;
  }

  rfftw_destroy_plan(plan_backward);

  print_epilogue();

  return 0;
}
