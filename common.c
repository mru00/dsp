#include "common.h"


extern float idx_to_freq(int i, float SRF, float NF) {
  return (double)i * 2.0 * SRF/NF;
}


extern void die(const char* msg) {
  perror(msg);
  exit(1);
}
		
// return the index in the spectrum for a 
// given frequency
extern int freq_to_idx(float freq, float SRF, float NF) {
  return freq * NF/ (SRF*2.0);
}


extern void print_prologoue(int N, int SR) {
  fprintf(stderr, "starting (%s) N:%d SR:%d SS:%d\n",
		  __BASE_FILE__, N, SR, SAMPLE_SIZE);
}

extern void print_epilogue() {
  fprintf(stderr, "terminating (%s)\n", __BASE_FILE__);
}
