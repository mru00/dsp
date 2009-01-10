#pragma once


typedef struct midi_file_t {

  FILE* handle;
  unsigned long difftime;
  byte_t running_status;
  long track_chunksize_fixup;

} midi_file_t;


extern midi_file_t* 
midi_write_open(const char* filename);


extern void 
midi_write_close(midi_file_t* file);

extern void 
midi_write_var_length(unsigned long number, 
					  midi_file_t* midi_file);

extern void 
midi_write_note_event(byte_t type, 
					  byte_t note, 
					  byte_t velocity, 
					  midi_file_t* midi_file);

extern void 
midi_write_header(short time_division, 
				  short number_of_tracks, 
				  midi_file_t* midi_file);

extern void 
midi_write_increase_difftime(unsigned long difftime, midi_file_t* file);

extern void 
midi_write_track_header(midi_file_t* midi_file);


extern void
midi_write_track_end(midi_file_t* midi_file);
