/*
 * applies convolution to input signal
 *
 *
 * kernel-file can be reread during runtime by sending a SIGUSR1 to the 
 * process
 *
 * input: read from stdin
 * output: written to stdout
 *
 */


/* problems:
 * normalize_kernel is wrong, too lazy to think
 */

/*
 * only mono works
 * sampling rate irrelevant
 * sample size defined by SAMPLE_SIZE
 * encoding must be PCM
 * kernel size constant by now
 */

#include <signal.h>


#include "common.h"


const int N=4096;
const int SR = 44100;


// No-op kernel
static float kernel[] = { 0.0, 0.0, 1.0, 0.0, 0.0 };

static char kernel_filename[FILENAME_MAX];

#define KERNEL_SIZE (sizeof(kernel)/sizeof(kernel[0]))


// still failing
static void normalize_kernel() {
  float sum = 0.0f;
  int i;

  for (i = 0; i< KERNEL_SIZE; i++ ) sum += kernel[i];
  sum = abs(sum);
  if (sum != 0.0f)
	for (i = 0; i< KERNEL_SIZE; i++ ) kernel[i] /= sum;
}




static void print_kernel() {
  int i;
  for (i = 0; i< KERNEL_SIZE-1; i++ ) 
	fprintf(stderr, "%f,", kernel[i]);
  fprintf(stderr, "%f\n", kernel[i]);
}


static void read_kernel(const char* filename) {

  int i;
  FILE* kernel_file = fopen(filename, "rt");

  // make a copy to enable SIGHUP reloading
  strcpy (kernel_filename, filename);

  // the usual error checking
  if ( kernel_file == NULL ) 
	die("Failed to read kernel");

  // read the kernel
  for (i = 0; i< KERNEL_SIZE; i++ ) 
	fscanf(kernel_file, "%f", &kernel[i]);

  fclose(kernel_file);

}


// send sigusr1 to reread kernel
static void on_sigusr1(int sig) {
  fprintf(stderr, "\nrereading kernel on signal %d\n", sig);
  read_kernel(kernel_filename);
  signal(SIGUSR1, on_sigusr1);
}


int main(int argc, char** argv) {

  int rd;
  buffer_t input;
  buffer_t buffer[KERNEL_SIZE];
  float output;
  int i;
  long bytes = 0;

  if (argc == 2) 
	read_kernel(argv[1]);

  print_prologoue(N, SR);

  print_kernel();
  normalize_kernel();
  print_kernel();

  signal(SIGUSR1, on_sigusr1);

  while (1) {

	// read sample
	rd = read(0, &input, sizeof(buffer_t)); 
	
	// eof
	if ( rd != sizeof(buffer_t) ) {
	  fprintf(stderr, "%li bytes processed, exiting (convolve)\n", bytes);
	  break;
	}

	// shift buffer; forward one sample
	for ( i = KERNEL_SIZE -1; i > 0; i-- )
	  buffer[i] = buffer[i-1];
	
	// append new sample
	buffer[0] = input;
	
	// convolve buffer
	output = 0.0f;
	for (i=0; i < KERNEL_SIZE; i++ )
	  output += buffer[i] * kernel[i];

	// output
	input = output;
	write(1, &input, rd);
	bytes += rd;
  } 

  print_epilogue();
  return 0;
}
