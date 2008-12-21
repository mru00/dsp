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


#define SAMPLE_SIZE 1


/* #define SAMPLE_SIZE 1 */
/* #define SR 44100 */
/* #define NF (float)N */
/* #define SRF (float)SR */



typedef unsigned char byte_t;
typedef unsigned int dword_t;
typedef unsigned short word_t;


#if SAMPLE_SIZE == 1
typedef byte_t buffer_t;
#elif SAMPLE_SIZE ==2
typedef word_t buffer_t;
#else
# error "unhandled buffer type"
#endif




extern void die(const char* msg);
		
// return the index in the spectrum for a 
// given frequency
extern int freq_to_idx(float freq, float SRF, float NF);
extern float idx_to_freq(int i, float SRF, float NF);


extern void print_prologoue(int N, int SR);
extern void print_epilogue();

// midi is wrong byte order
static inline word_t reorder_word(word_t in) {
  return ((in>>8)&0xff) | ((in&0xff) << 8);
}

static inline dword_t reorder_dword(dword_t in) {
  return reorder_word((in >> 16)&0xffff) | reorder_word(in&0xffff);
}
