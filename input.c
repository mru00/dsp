#include "common.h"
#include "input.h"
#include <alsa/asoundlib.h>


#define WHAT_STRING_SIZE 80



static char what[WHAT_STRING_SIZE];


/* ------------ stdin */

static size_t 
input_read_stdin(float* buffer, const size_t count) {
  return read(0, buffer, count*sizeof(float));
}

/* ------------ alsa */

snd_pcm_t *capture_handle;

static void 
alsa_die(int err, const char* msg) {
  fprintf (stderr, "%s (%s = 0x%x = %d)\n", msg, snd_strerror (err), err, err);
  exit (1);
}

#define ALSA_CHECKED(x, y) if ( (err = (x)) < 0 ) alsa_die(err, (y));

static void
input_open_alsa(const char* dev) {
  int err;
  unsigned int rate = 44100;
  snd_pcm_hw_params_t *hw_params;
  
  ALSA_CHECKED(snd_pcm_open (&capture_handle, dev, 
							 SND_PCM_STREAM_CAPTURE, 0),
			   "cannot open audio device");
		   
  ALSA_CHECKED(snd_pcm_hw_params_malloc (&hw_params),
			   "cannot allocate hardware parameter structure");
				 
  ALSA_CHECKED(snd_pcm_hw_params_any (capture_handle, hw_params),
			   "cannot initialize hardware parameter structure");
  
  ALSA_CHECKED(snd_pcm_hw_params_set_access (capture_handle, 
											 hw_params, 
											 SND_PCM_ACCESS_RW_INTERLEAVED),
			   "cannot set access type");
	
  ALSA_CHECKED(snd_pcm_hw_params_set_format (capture_handle, 
											 hw_params, 
											 SND_PCM_FORMAT_FLOAT),
			   "cannot set sample format");
	
  ALSA_CHECKED(snd_pcm_hw_params_set_rate_near (capture_handle, 
												hw_params, 
												&rate, 0),
			   "cannot set sample rate");
	
  ALSA_CHECKED(snd_pcm_hw_params_set_channels (capture_handle, 
											   hw_params, 1),
			   "cannot set channel count");
	
  ALSA_CHECKED(snd_pcm_hw_params (capture_handle, hw_params),
			   "cannot set parameters");
	
  snd_pcm_hw_params_free (hw_params);
	
  ALSA_CHECKED(snd_pcm_prepare (capture_handle),
			   "cannot prepare audio interface for use");
}
  
static size_t
input_read_alsa(float* buffer, size_t count) {
  
  int err;

  if ((err = snd_pcm_readi (capture_handle, buffer, count)) != count)
	alsa_die (err, "read from audio interface failed ");
  
  return err;
}
  
static void 
input_close_alsa() {
  snd_pcm_close (capture_handle);
}





extern rdfun
input_open(const char* which) {

  strncpy(what, which, WHAT_STRING_SIZE);

  if ( strcmp(which, "stdin") == 0 ) {
	return &input_read_stdin;
  } 
  if ( strncmp(which, "alsa", 4) == 0 ) {
	input_open_alsa(which+5);
	return &input_read_alsa;
  }

  die("unknown input");
  
  return NULL;
}


extern void
input_close() {

  if ( strcmp(what, "stdin") == 0 ) {
  } 
  else if ( strncmp(what, "alsa", 4) == 0 ) {
	input_close_alsa();
  }
  else {
	// die...
  }
}

