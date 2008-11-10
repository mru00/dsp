#pragma once

/*
 "Debugging is twice as hard as writing the code in the first place.
  Therefore, if you write the code as cleverly as possible, you are,
  by definition, not smart enough to debug it."
   --Brian Kernighan
*/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>


#ifndef N
#error "#define N before including this file!"
#endif


#define SAMPLE_SIZE 1
#define SR 22050
#define NF (float)N
#define SRF (float)SR



typedef unsigned char byte_t;
typedef unsigned int dword_t;
typedef unsigned short word_t;


#if SAMPLE_SIZE == 1
typedef char buffer_t;
#elif SAMPLE_SIZE ==2
typedef short buffer_t;
#else
# error "unhandled buffer type"
#endif

/*
 * never do this at home:
 *
 * i directly put some functions into the header
 *
 * not nice, but simplyfies the make procedure
 *
 */

extern float idx_to_freq(int i) {
  return (double)i * 2.0 * SRF/NF;
}


extern void die(const char* msg) {
  perror(msg);
  exit(1);
}
		
// return the index in the spectrum for a 
// given frequency
extern  int freq_to_idx(float freq) {
  return freq * NF/ (SRF*2.0);
}
