#include "common.h"

#include "midifile.h"



extern midi_file_t* 
midi_write_open(const char* filename) {

  midi_file_t* file = (midi_file_t*) malloc(sizeof(midi_file_t));

  file->handle = fopen (filename, "wb");
  file->difftime = 0;
  file->running_status = 0xff;

  return file;
}


extern void 
midi_write_close(midi_file_t* file) {

  fclose(file->handle);
  free(file);
}


extern void 
midi_write_increase_difftime(unsigned long difftime, midi_file_t* file) {

  file->difftime += difftime;

}

//http://www.omega-art.com/midi/mfiles.html
extern void 
midi_write_var_length(unsigned long number, midi_file_t* file) {
  byte_t b;
  
  unsigned long buffer;
  buffer = number & 0x7f;
  while ((number >>= 7) > 0) {
	buffer <<= 8;
	buffer |= 0x80 + (number & 0x7f);
  }
  while (1) {
	b = buffer & 0xff;
	fwrite(&b, 1, 1, file->handle);
	
	if (buffer & 0x80) buffer >>= 8;
	else break;
  }
}



extern void 
midi_write_note_event(byte_t type, 
					  byte_t note, 
					  byte_t velocity, 
					  midi_file_t* file) {

  midi_write_var_length ( file->difftime, file );
  file->difftime = 0;

  // running status
  if ( type != file->running_status ) {
	fwrite(&type, 1, 1, file->handle);
	file->running_status = type;
  }

  fwrite(&note, 1, 1, file->handle);
  fwrite(&velocity, 1, 1, file->handle);
}



extern void 
midi_write_header(short timedivision, 
				  short numberoftracks, 
				  midi_file_t* file) {

  assert (file != NULL);
  int chunksize = reorder_dword(6);
  short formattype = reorder_word(1);

  numberoftracks  = reorder_word(numberoftracks);
  timedivision= reorder_word(timedivision);

  fwrite("MThd", 4, 1, file->handle);
  fwrite(&chunksize, 4, 1, file->handle);
  fwrite(&formattype, 2, 1, file->handle);
  fwrite(&numberoftracks, 2, 1, file->handle);
  fwrite(&timedivision, 2, 1, file->handle);
}



extern void 
midi_write_track_header(midi_file_t* file) {
  assert (file->handle != NULL);

  int chunksize = reorder_dword(60000);

  fwrite("MTrk", 4, 1, file->handle);
  file->track_chunksize_fixup = ftell(file->handle);
  fwrite(&chunksize, 4, 1, file->handle);
}


extern void
midi_write_track_end(midi_file_t* file) {
  byte_t b;

  midi_write_var_length ( 0, file );
  file->difftime = 0;

  b = 0xFF; fwrite(&b, 1, 1, file->handle);
  b = 0x2F; fwrite(&b, 1, 1, file->handle);
  midi_write_var_length(0, file);


  long pos = ftell(file->handle);
  int chunksize = reorder_dword(pos - file->track_chunksize_fixup - sizeof(dword_t));

  fseek(file->handle,file->track_chunksize_fixup, SEEK_SET);
  fwrite(&chunksize, 4, 1, file->handle);
  fseek(file->handle, pos, SEEK_SET);

}
