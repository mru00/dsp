/*
 * stupid song player
 *
 *
 * input: song data from song.txt
 * output: mono/22050/signed 8bit audio to stdout
 */

#include <rfftw.h>
#include <math.h>

#include "common.h"

// output volume multiplier
const float pregain = 2;
const int N=1024;
const int SR=44100;

#define ACCORD_SIZE 5


// song is organized as a linked list
//
typedef struct tag_song_t  {
  struct tag_song_t* next;
  int duration;
  int accord[ACCORD_SIZE];
} song_t;



// attac decay sustain release
// returns a factor between 0 and 1
/*
 *       ^                     - 1.0
 *	    / \
 * 	   /   \
 * 	  /	    \_____________     - sustain level
 *   /					 \
 *  /                     \
 * /                       \   - 0.0
 *
 * |-A-|-D-|-----S------|-R-|
 */

float ADSR(int i, int duration) {

  float p = (float)i/(float)duration;

  const float sustain = 0.9;
  const float ad = 0.1;
  const float ds = 0.5;
  const float sr = 0.85;

  // attack
  if ( p <= ad ) return p/ad;
  // decay
  if ( p <= ds ) return 1.0 + (p-ad)*(-(1.0-sustain)/(ds-ad));
  // sustain
  else if ( p < sr ) return sustain;
  // release
  return sustain + (p-sr) * (-sustain / (1.0-sr));
}


void dump_song(const song_t* head) {
  int i;
  while ( head != NULL ) {

	i = 0;
	fprintf(stderr, "%5d (", head-> duration);
	while (i < ACCORD_SIZE && head->accord[i] != 0)
	  fprintf(stderr, "%4d ", head->accord[i++]);
	fprintf(stderr, ")\n");

	head = head -> next;
  }
}


// read song-file
song_t* read_song(const char* filename) {

  song_t* head = NULL;
  song_t* next = NULL;
  song_t* last;
  float note;
  int i;
  int retval;

  FILE* song_file = fopen(filename, "rt");

  if (song_file == NULL)
	die ("Failed to read song");

  while ( !feof(song_file) ) {

	last = next;
	next = (song_t*) malloc(sizeof(song_t));
	if (last != NULL) last->next = next;
	if (head == NULL) head = next;
	next->next = NULL;

	fscanf(song_file, "%d", &next->duration);
	fscanf(song_file, " ( ");

	i = 0;
	do {
		retval = fscanf(song_file, "%f", &note);
		next->accord[i++] = freq_to_idx(note, SR, N);
	} while ( retval != 0 );

	fscanf(song_file, " ) ");
  }

  return head;
}

// free song data
void free_song(song_t* head) {
  song_t* tmp;
  while (head != NULL) {
	tmp = head->next;
	free(head);
	head = tmp;
  }
}

// find good ck's for instruments!!!

// value ( ck )
float cr(int i) {
  float f = (float)i;

  return expf(-(f-1)/100) * 0.7*cosf(f*3.1415/2.0) + 0.3*sinf(f*3.1415/2.0);

  return 0.7*cosf(f*10) + 0.3*sinf(f*10);
  return 0.5;
  return cos(f*3.1415/2.0);
  return expf(-(f-1)/100) * 0.7*cosf(f*3.1415/2.0) + 0.3*sinf(f*3.1415/2.0);
  return expf(-(f-1)/100) * 0.7*sinf(f*3.1415/2.0) + 0.3*cosf(f*3.1415/2.0);
  return expf(-(f-1)/100) * sinf(f*3.1415/2.0);
}

// arg( ck )
// todo: any ideas?
float cp(int i) {
  return 0.0;
}


// add a single note to spectrum
void play_note(int freq, float adsr, fftw_real* out) {

  int i=1;
  float r, p;

  while ( i*freq < N/2 ) {
	r = cr(i);
	p = cp(i);

	// re{ck}
	out[freq*i]   += adsr * N * r*cos(p);

	// im{ck}
	out[N-freq*i] += adsr * N * r*sin(p);

	i++;
  }
}

// todo: implement limiting!
buffer_t limit_output(float f) {
  return pregain * f/(float)N;
}

int main(int argc, char** argv) {

  fftw_real out[N], in[N];
  rfftw_plan plan_backward;
  buffer_t buffer[N];

  int i, time = 0;

  song_t* head;
  song_t* next;

  print_prologoue(N, SR);

  next = head = read_song("song.txt");
  dump_song(head);
  plan_backward = rfftw_create_plan(N, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);

  while ( next ) {

	// clear out
	for ( i = 0; i < N; i++ )
		out[i] =  0.0f;

	i = 0;
	while (i < ACCORD_SIZE && next->accord[i] != 0)
	  play_note(next->accord[i++], ADSR(time, next->duration), out);

	rfftw_one(plan_backward, out, in);

	for ( i = 0; i < N; i++ )
	  buffer[i] = limit_output(in[i]);;

  	write(1, buffer, N* sizeof(buffer_t));

	time ++;
	if ( time == next->duration ) {
	  // play next note

	  next = next->next;
	  time = 0;

	  // loop:
	  if (next == NULL) next = head;
	}
  }

  rfftw_destroy_plan(plan_backward);
  free_song(head);
  print_epilogue();
  return 0;
}
