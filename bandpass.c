#include <rfftw.h>
#include <signal.h>

#define N (4096<<0)

#include "common.h"


int main(int argc, char** argv) {

  fftw_real out[N], in[N];
  rfftw_plan plan_forward, plan_backward;
  buffer_t buffer[N];

  float lofreq, hifreq;
  int loidx, hiidx;

  int rd;
  int i;
  long bytes = 0;

  if ( argc != 3 )
	die("usage: bandpass lofreq hifreq");

  sscanf(argv[1], "%f", &lofreq);
  sscanf(argv[2], "%f", &hifreq);

  loidx = freq_to_idx(lofreq);
  hiidx = freq_to_idx(hifreq);

  assert (loidx < hiidx);
  assert (loidx >= 0);
  assert (hiidx < N/2);

  plan_forward = rfftw_create_plan(N, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
  plan_backward = rfftw_create_plan(N, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);


  while (1) {

	// read sample
	rd = read(0, buffer, N* sizeof(buffer_t)); 

	// eof
	if ( rd != N*sizeof(buffer_t) )  {
	  fprintf(stderr, "%li bytes processed, %d read, exiting (vocoder)\n", 
			  bytes, rd);
	  return 0;
	}
	

	for ( i = 0; i < N; i++ ) 
	  in[i] = buffer[i];

	rfftw_one(plan_forward, in, out);
	
	for ( i = 1; i < loidx; i++ )
	  out[i] = out[N-i] = 0.0;

	for ( i = hiidx; i < N/2; i++ )
	  out[i] = out[N-i] = 0.0;


	rfftw_one(plan_backward, out, in);

	for ( i = 0; i < N; i++ ) 
	  buffer[i] = in[i]/N;

  	write(1, buffer, N* sizeof(buffer_t));
	bytes += rd;
  }

  rfftw_destroy_plan(plan_forward);
  rfftw_destroy_plan(plan_backward);

  return 0;
}
