#include "common.h"
#include "input.h"

#define WHAT_STRING_SIZE 80



static char what[WHAT_STRING_SIZE];
static rdfun rd_function = NULL;
static size_t buffer_size = 0;
static buffer_t* buffer;

static size_t 
input_read_stdin( buffer_t* buffer, const size_t size) {
  return read(0, buffer, buffer_size*sizeof(buffer_t));
}

extern buffer_t*
input_open(const char* which, const size_t size) {

  strncpy(what, which, WHAT_STRING_SIZE);
  buffer_size = size;
  buffer = (buffer_t*)malloc ( size * sizeof(buffer_t*) );

  if ( strcmp(which, "stdin") == 0 ) {
	rd_function = input_read_stdin;
	return buffer;
  } 

  die("unknown input");
  
  return NULL;
}

extern int
input_read() {
  
  assert (rd_function != NULL);
  assert (buffer != NULL);

  return rd_function(buffer, buffer_size);
}

extern void
input_close() {
  free(buffer);
  buffer = NULL;
  rd_function = NULL;


  if ( strcmp(what, "stdin") == 0 ) {
  } 
  else {
	// die...
  }
}

