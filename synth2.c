/*
 * synthesizes a note using ck-values read from a file
 *
 * ck_filename = cks.txt
 *
 * base frequency = F_BASE
 *
 * ck-file can be reread during runtime by sending a SIGUSR1 to the 
 * process
 *
 * make && ./synth2 | play -s -1 -c 1 -t raw -r 44100 -
 *
 *
 * input: read from stdin
 * output: written to stdout
 *
 */

#include <rfftw.h>
#include <signal.h>

#define N (4096<<0)

#include "common.h"


#define CK_SIZE 10      // number of ck's 
#define F_BASE 30

#define CKS_FILENAME "cks.txt"

float ck[CK_SIZE];


void read_ck_file(const char* filename) {

  int i;
  FILE* ck_file = fopen(filename, "rt");

  if ( ck_file == NULL ) 
	die("Failed to read cks");

  for (i = 0; i< CK_SIZE; i++ ) 
	fscanf(ck_file, "%f", ck +i);

  fclose(ck_file);
  fprintf(stderr, "read ck_file\n");

}

void on_sigusr1(int sig) {
  fprintf(stderr, "\nrereading cks on signal %d\n", sig);
  read_ck_file(CKS_FILENAME);
  signal(SIGUSR1, on_sigusr1);
}


int main(void) {


  fftw_real out[N], in[N];
  rfftw_plan plan_backward;
  buffer_t buffer[N];

  int i, rd;
  long bytes = 0;

  plan_backward = rfftw_create_plan(N, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);

  read_ck_file("cks.txt");

  signal(SIGUSR1, on_sigusr1);

  while ( 1 ) {

	out[0] = 0.0;

	for ( i = 1; i < N/2; i++ ) {
	  if ( i % F_BASE == 0 ){
		out[i] = N*ck[i/F_BASE -1];
		out[N-i] = N*ck[i/F_BASE -1];
	  }
	  else {
		out[i] = 0.0;
		out[N-i] = 0.0f;
	  }
	}

	rfftw_one(plan_backward, out, in);


	for ( i = 0; i < N; i++ ) 
	  buffer[i] = in[i]/(N*2);

  	write(1, buffer, N* sizeof(buffer_t));
	bytes += rd;


  }


  rfftw_destroy_plan(plan_backward);

  return 0;
}
