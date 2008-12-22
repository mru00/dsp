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

static const double pi = 3.1415;
static const double sec_per_min = 60.0;


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
  return (reorder_word((in >> 16)&0xffff)) | (reorder_word(in&0xffff)<<16);
}

static inline double 
midi_note_to_hertz(int note) {
  // i have no idea where the 2* comes from
  return 2*(440.0/32.0) * pow(2.0, (double)(note-9)/12.0);
}

static inline double 
hertz_to_radians_per_sec(double hertz) {
  return 2.0 * hertz * pi;
}

static inline double 
radians_per_sec_to_radians_per_sample(double SR, double radians) {
  return radians/SR;
}

static inline double 
midi_note_to_radians_per_sample(double SR, int note) {
  double r = midi_note_to_hertz(note);
  r = hertz_to_radians_per_sec(r);
  return radians_per_sec_to_radians_per_sample(SR, r);
}

static inline unsigned int 
samples_per_midi_tick(unsigned SR, double bpm, double timeDivision) {
  return SR * sec_per_min / (bpm * timeDivision);
}

