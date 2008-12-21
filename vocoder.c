#include <rfftw.h>

#include "common.h"

const int N=4096;
const int SR = 44100;

#define F_BASE 2


int main(void) {

  float ck[N/2];
  fftw_real in[N], out[N];
  rfftw_plan plan_forward, plan_backward;
  buffer_t buffer[N];

  print_prologoue(N, SR);

  int i, rd;
  long bytes = 0;


  plan_forward = rfftw_create_plan(N, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
  plan_backward = rfftw_create_plan(N, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);

  for ( i = 1; i < N/2; i++ ) {
	if ( i % F_BASE == 0 )  ck[i] = 1.0;
	else ck[i] = 0.0;
  }

  while ( 1 ) {
	// read sample
	rd = read(0, buffer, N* sizeof(buffer_t)); 

	// eof
	if ( rd != N*sizeof(buffer_t) )  {
	  fprintf(stderr, "%li bytes processed, %d read, exiting (vocoder)\n", 
			  bytes, rd);
	  break;
	}

	for ( i = 0; i < N; i++ ) 
	  in[i] = buffer[i];

	rfftw_one(plan_forward, in, out);
	
	for ( i = 1; i < N/2; i++ ) {
	  out[i] *= ck[i];
	  out[N-i] *= ck[i];
	}

	rfftw_one(plan_backward, out, in);


	for ( i = 0; i < N; i++ ) 
	  buffer[i] = in[i]/N;

  	write(1, buffer, N* sizeof(buffer_t));
	bytes += rd;
  }


  rfftw_destroy_plan(plan_forward);
  rfftw_destroy_plan(plan_backward);

  print_epilogue();

  return 0;
}
