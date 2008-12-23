#include <stdlib.h>
#include <math.h>

#include "common.h"

const int N = 4096;
const int SR = 44100;

/*
 *   0x00	4	char[4]	chunk ID	"MThd" (0x4D546864)
 *   0x04	4	dword	chunk size	6 (0x00000006)
 *   0x08	2	word	format type	0 - 2
 *   0x10	2	word	number of tracks	1 - 65,535
 *   0x12	2	word	time division	see following text
 */
typedef struct tag_midi_header {
  byte_t  chunkID[4];
  dword_t chunkSize;
  word_t  formatType;
  word_t  numberOfTracks;
  word_t  timeDivision;
} midi_header_t;


/*
 *   0x00	4	char[4]	chunk ID	"MTrk" (0x4D54726B)
 *   0x04	4	dword	chunk size	see following text
 *   0x08	track event data (see following text)
 */
typedef struct tag_track_header {
  byte_t  chunkID[4];
  dword_t chunkSize;

  byte_t* eventData;
  byte_t* nextEvent;

  unsigned long decrementedDiffTime;
  int     trackID;
  byte_t  done;

  byte_t status;
} track_header_t;


FILE*         midi_file = NULL;
midi_header_t midi_header;

static byte_t* read_var_length(byte_t* ptr, unsigned long* result);
static byte_t* print_midi_event(byte_t status, byte_t* data);
static byte_t* print_meta_event(byte_t* data);
static byte_t* print_sysex_event(byte_t* data);


// parse midi header
static void read_midi_header() {

  assert (midi_file != NULL);

  fread(&midi_header.chunkID, 4, 1, midi_file);
  fread(&midi_header.chunkSize, 4, 1, midi_file);
  fread(&midi_header.formatType, 2, 1, midi_file);
  fread(&midi_header.numberOfTracks, 2, 1, midi_file);
  fread(&midi_header.timeDivision, 2, 1, midi_file);

  // adapt byte-order
  midi_header.formatType     = reorder_word(midi_header.formatType);
  midi_header.numberOfTracks = reorder_word(midi_header.numberOfTracks);
  midi_header.timeDivision   = reorder_word(midi_header.timeDivision);
  midi_header.chunkSize      = reorder_dword(midi_header.chunkSize);
}


// hr-values
static void print_midi_header() {
  printf(
		  "midi header :\n"
		  "  chunkID:      %c%c%c%c\n"
		  "  chunkSize:    %u\n"
		  "  formatType    %u\n"
		  "  #Tracks:      %u\n"
		  "  timeDivision: %u\n",
		  midi_header.chunkID[0],
		  midi_header.chunkID[1],
		  midi_header.chunkID[2],
		  midi_header.chunkID[3],
		  midi_header.chunkSize,
		  midi_header.formatType,
		  midi_header.numberOfTracks,
		  midi_header.timeDivision);
}

// parse track header
static void read_track_header(track_header_t* hdr) {
  assert (midi_file != NULL);
  assert (hdr != NULL);

  fread(&hdr->chunkID, 4, 1, midi_file);
  fread(&hdr->chunkSize, 4, 1, midi_file);

  hdr->chunkSize = reorder_dword(hdr->chunkSize);
  hdr->eventData = (byte_t*) malloc(hdr->chunkSize);
  hdr->nextEvent = hdr->eventData;
  hdr->done = 0;

  fread(hdr->eventData, hdr->chunkSize, 1, midi_file);

  // preload decrementedDiffTime
  read_var_length(hdr->eventData, &hdr->decrementedDiffTime);
}


// hr-values
static void print_track_header(const track_header_t* hdr) {
  assert (hdr != NULL);

  printf(
		  "track header:\n"
		  "  chunkID:      %c%c%c%c\n"
		  "  chunkSize:    %u\n",
		  hdr->chunkID[0],
		  hdr->chunkID[1],
		  hdr->chunkID[2],
		  hdr->chunkID[3],
		  hdr->chunkSize);
}

// returns the number of bytes read
// *result will get length
static byte_t* read_var_length(byte_t* ptr, unsigned long* result) {

  int i = 0;
  unsigned long tmp = 0;
  byte_t b;

  do {
	b = *(ptr + i++);
	tmp = (tmp << 7) + ( b & 0x7f );
  } while ( b & 0x80 );

  *result = tmp;
  return ptr+i;
}


static void parse_event_data(track_header_t* hdr) {

  byte_t* ptr = hdr->eventData;
  unsigned long difftime;
  static byte_t status;

  while ( ptr < (hdr->eventData+hdr->chunkSize) ) {

	// difftime
	ptr = read_var_length(ptr, &difftime);

	printf("difftime: %-6lu", difftime);

	if ( (*ptr & 0x80) == 0x80 )
	  status = *(ptr++);

	// meta event
	if ( status == 0xff ) {
	  ptr = print_meta_event(ptr);
	}
	// sysex event
	else if ( status == 0xF0  || status == 0xF7 ) {
	  ptr = print_sysex_event(ptr);
	}
	// "normal midi event
	else {
	  ptr = print_midi_event(status, ptr);
	}
  }
}


// hr-values
static byte_t* print_meta_event(byte_t* data) {

  printf ( " meta: ");

  byte_t eventType = *(data++);
  unsigned long length;
	  
  data = read_var_length(data, &length);

  switch(eventType) {
  case 0x00: printf("sequence number: %u\n", 
					 (data[0]<<8)+data[1]); 
	break;
  case 0x01: printf("text message: %s\n", data); break;
  case 0x02: printf("copyright notice: %s\n", data); break;
  case 0x03: printf("track name: %s\n", data);	break;
  case 0x04: printf("instrument name: %s\n", data); break;
  case 0x05: printf("lyric: %s\n", data); break;
  case 0x06: printf("marker: %s\n", data); break;
  case 0x07: printf("cue point: %s\n", data); break;
  case 0x21: printf("midi port %d\n", 
					 data[0]); break;
  case 0x2F: printf("end of track\n"); break;
  case 0x51: printf("set tempo %u (bpm = %f)\n", 
					 ((data[0]<<16)|(data[1]<<8)|(data[2])),
					 60000000.0/(((data[0]<<16)|(data[1]<<8)|(data[2])))); 
	break;
  case 0x54: printf("SMPTE Offset hr:%d mn:%d se:%d fr:%d ff:%d\n",
					 data[0], data[1], data[2], data[3], data[4]); 
	break;
  case 0x58: printf("time signature: nn:%d dd:%d cc:%d bb:%d\n",
					 data[0], data[1], data[2], data[3]); 
	break;
  case 0x59: printf("key signature: sf:%d mi:%d\n", 
					data[0], data[1]); 
	break;
  case 0x7F: printf("sequencer-specific\n"); 
	break;
  default: printf("unknown (type = 0x%x)\n", eventType);
  }

  return data + length;
}


static void print_controller_msg(byte_t p1, byte_t p2) {

  printf("Controller :%d  value:%d description: ", p1,p2);

  switch (p1) {
  case 0x00: printf("Bank Select"); break;
  case 0x01: printf("Modulation Wheel or Lever"); break;
  case 0x02: printf("Breath Controller"); break;
  case 0x03: printf("Undefined"); break;
  case 0x04: printf("Foot Controller"); break;
  case 0x05: printf("Portamento Time"); break;
  case 0x06: printf("Data Entry MSB"); break;
  case 0x07: printf("Channel Volume (formerly Main Volume)"); break;
  case 0x08: printf("Balance"); break;
  case 0x09: printf("Undefined"); break;
  case 0x0A: printf("Pan"); break;
  case 0x0B: printf("Expression Controller"); break;
  case 0x0C: printf("Effect Control 1"); break;
  case 0x0D: printf("Effect Control 2"); break;
  case 0x0E: printf("Undefined"); break;
  case 0x0F: printf("Undefined"); break;
  case 0x10: printf("General Purpose Controller 1"); break;
  case 0x11: printf("General Purpose Controller 2"); break;
  case 0x12: printf("General Purpose Controller 3"); break;
  case 0x13: printf("General Purpose Controller 4"); break;
  case 0x14: printf("Undefined"); break;
  case 0x15: printf("Undefined"); break;
  case 0x16: printf("Undefined"); break;
  case 0x17: printf("Undefined"); break;
  case 0x18: printf("Undefined"); break;
  case 0x19: printf("Undefined"); break;
  case 0x1A: printf("Undefined"); break;
  case 0x1B: printf("Undefined"); break;
  case 0x1C: printf("Undefined"); break;
  case 0x1D: printf("Undefined"); break;
  case 0x1E: printf("Undefined"); break;
  case 0x1F: printf("Undefined"); break;
  case 0x20: printf("LSB for Control 0 (Bank Select)"); break;
  case 0x21: printf("LSB for Control 1 (Modulation Wheel or Lever)"); break;
  case 0x22: printf("LSB for Control 2 (Breath Controller)"); break;
  case 0x23: printf("LSB for Control 3 (Undefined)"); break;
  case 0x24: printf("LSB for Control 4 (Foot Controller)"); break;
  case 0x25: printf("LSB for Control 5 (Portamento Time)"); break;
  case 0x26: printf("LSB for Control 6 (Data Entry)"); break;
  case 0x27: printf("LSB for Control 7 (Channel Volume, formerly Main Volume)"); break;
  case 0x28: printf("LSB for Control 8 (Balance)"); break;
  case 0x29: printf("LSB for Control 9 (Undefined)"); break;
  case 0x2A: printf("LSB for Control 10 (Pan)"); break;
  case 0x2B: printf("LSB for Control 11 (Expression Controller)"); break;
  case 0x2C: printf("LSB for Control 12 (Effect control 1)"); break;
  case 0x2D: printf("LSB for Control 13 (Effect control 2)"); break;
  case 0x2E: printf("LSB for Control 14 (Undefined)"); break;
  case 0x2F: printf("LSB for Control 15 (Undefined)"); break;
  case 0x30: printf("LSB for Control 16 (General Purpose Controller 1)"); break;
  case 0x31: printf("LSB for Control 17 (General Purpose Controller 2)"); break;
  case 0x32: printf("LSB for Control 18 (General Purpose Controller 3)"); break;
  case 0x33: printf("LSB for Control 19 (General Purpose Controller 4)"); break;
  case 0x34: printf("LSB for Control 20 (Undefined)"); break;
  case 0x35: printf("LSB for Control 21 (Undefined)"); break;
  case 0x36: printf("LSB for Control 22 (Undefined)"); break;
  case 0x37: printf("LSB for Control 23 (Undefined)"); break;
  case 0x38: printf("LSB for Control 24 (Undefined)"); break;
  case 0x39: printf("LSB for Control 25 (Undefined)"); break;
  case 0x3A: printf("LSB for Control 26 (Undefined)"); break;
  case 0x3B: printf("LSB for Control 27 (Undefined)"); break;
  case 0x3C: printf("LSB for Control 28 (Undefined)"); break;
  case 0x3D: printf("LSB for Control 29 (Undefined)"); break;
  case 0x3E: printf("LSB for Control 30 (Undefined)"); break;
  case 0x3F: printf("LSB for Control 31 (Undefined)"); break;
  case 0x40: printf("Damper Pedal on/off (Sustain)	<63 off, >64 on"); break;
  case 0x41: printf("Portamento On/Off	<63 off, >64 on"); break;
  case 0x42: printf("Sostenuto On/Off	<63 off, >64 on"); break;
  case 0x43: printf("Soft Pedal On/Off	<63 off, >64 on"); break;
  case 0x44: printf("Legato Footswitch	<63 Normal, >64 Legato"); break;
  case 0x45: printf("Hold 2	<63 off, >64 on"); break;
  case 0x46: printf("Sound Controller 1 (default: Sound Variation)"); break;
  case 0x47: printf("Sound Controller 2 (default: Timbre/Harmonic Intens.)"); break;
  case 0x48: printf("Sound Controller 3 (default: Release Time)"); break;
  case 0x49: printf("Sound Controller 4 (default: Attack Time)"); break;
  case 0x4A: printf("Sound Controller 5 (default: Brightness)"); break;
  case 0x4B: printf("Sound Controller 6 (default: Decay Time - see MMA RP-021)"); break;
  case 0x4C: printf("Sound Controller 7 (default: Vibrato Rate - see MMA RP-021)"); break;
  case 0x4D: printf("Sound Controller 8 (default: Vibrato Depth - see MMA RP-021)"); break;
  case 0x4E: printf("Sound Controller 9 (default: Vibrato Delay - see MMA RP-021)"); break;
  case 0x4F: printf("Sound Controller 10 (default undefined - see MMA RP-021)"); break;
  case 0x50: printf("General Purpose Controller 5"); break;
  case 0x51: printf("General Purpose Controller 6"); break;
  case 0x52: printf("General Purpose Controller 7"); break;
  case 0x53: printf("General Purpose Controller 8"); break;
  case 0x54: printf("Portamento Control"); break;
  case 0x55: printf("Undefined"); break;
  case 0x56: printf("Undefined"); break;
  case 0x57: printf("Undefined"); break;
  case 0x58: printf("Undefined"); break;
  case 0x59: printf("Undefined"); break;
  case 0x5A: printf("Undefined"); break;
  case 0x5B: printf("Effects 1 Depth (default: Reverb Send Level - see MMA RP-023) (formerly External Effects Depth)"); break;
  case 0x5C: printf("Effects 2 Depth (formerly Tremolo Depth)"); break;
  case 0x5D: printf("Effects 3 Depth (default: Chorus Send Level - see MMA RP-023) (formerly Chorus Depth)"); break;
  case 0x5E: printf("Effects 4 Depth (formerly Celeste [Detune] Depth)"); break;
  case 0x5F: printf("Effects 5 Depth (formerly Phaser Depth)"); break;
  case 0x60: printf("Data Increment (Data Entry +1) (see MMA RP-018)	N/A"); break;
  case 0x61: printf("Data Decrement (Data Entry -1) (see MMA RP-018)	N/A"); break;
  case 0x62: printf("Non-Registered Parameter Number (NRPN) - LSB"); break;
  case 0x63: printf("Non-Registered Parameter Number (NRPN) - MSB"); break;
  case 0x64: printf("Registered Parameter Number (RPN) - LSB*"); break;
  case 0x65: printf("Registered Parameter Number (RPN) - MSB*"); break;
  case 0x66: printf("Undefined"); break;
  case 0x67: printf("Undefined"); break;
  case 0x68: printf("Undefined"); break;
  case 0x69: printf("Undefined"); break;
  case 0x6A: printf("Undefined"); break;
  case 0x6B: printf("Undefined"); break;
  case 0x6C: printf("Undefined"); break;
  case 0x6D: printf("Undefined"); break;
  case 0x6E: printf("Undefined"); break;
  case 0x6F: printf("Undefined"); break;
  case 0x70: printf("Undefined"); break;
  case 0x71: printf("Undefined"); break;
  case 0x72: printf("Undefined"); break;
  case 0x73: printf("Undefined"); break;
  case 0x74: printf("Undefined"); break;
  case 0x75: printf("Undefined"); break;
  case 0x76: printf("Undefined"); break;
  case 0x77: printf("Undefined"); break;
  case 0x78: printf("[Channel Mode Message] All Sound Off	0"); break;
  case 0x79: printf("[Channel Mode Message] Reset All Controllers (See MMA RP-015)	0"); break;
  case 0x7A: printf("[Channel Mode Message] Local Control On/Off	0 off, 127 on"); break;
  case 0x7B: printf("[Channel Mode Message] All Notes Off	0"); break;
  case 0x7C: printf("[Channel Mode Message] Omni Mode Off (+ all notes off)	0"); break;
  case 0x7D: printf("[Channel Mode Message] Omni Mode On (+ all notes off)	0"); break;
  case 0x7E: printf("[Channel Mode Message] Mono Mode On (+ poly off +all notes off)	**"); break;
  case 0x7F: printf("[Channel Mode Message] Poly Mode On (+ mono off +all notes off)	0"); break;
  }

  printf("\n");
}

// hr-values
static byte_t* print_midi_event(byte_t status, byte_t* data) {

  byte_t eventType = (status >> 4) & 0xf;
  byte_t channel = status & 0xf;

  printf ( " midi: ch: %d ", channel);

  switch ( eventType ) {
  case 0x8: printf("Note Off note:%-3d velocity:%-3d\n", 
					data[0],data[1]); data += 2; break;
  case 0x9: printf("Note On  note:%-3d velocity:%-3d\n", 
					data[0],data[1]); data += 2; break;
  case 0xA: printf("Note Aftertouch (%d, %d)\n", 
					data[0],data[1]); data += 2; break;
  case 0xB: print_controller_msg(data[0],data[1]); 
	data += 2; break;
  case 0xC: printf("Program Change (%d)\n", 
					data[0]); data += 1; break;
  case 0xD: printf("Channel Aftertouch (%d)\n", 
					data[0]); data += 1; break;
  case 0xE: printf("Pitch Bend (%d, %d)\n", 
					data[0],data[1]); data += 2; break;
  default:	printf("unknown (type = 0x%x, ch=%d, p1=%d, p2=%d)\n", 
					eventType, channel, data[0],data[1]); data += 2;
  }

  return data;
}

static byte_t* print_sysex_event(byte_t* data) {

  unsigned long length;
  data = read_var_length(data, &length);
  printf ( " sysex: \n");
  return data + length;
}


int main(int argc, char** argv) {

  track_header_t* track_headers;
  int i;


  if (argc == 1)
	midi_file = stdin;
  else if ( argc == 2 )
	midi_file = fopen(argv[1], "rb");
  else 
	die("usage: midiparse [midifile]");

  


  if (midi_file == NULL)
	die("failed to open midifile");

  print_prologoue(N, SR);

  read_midi_header(&midi_header);
  print_midi_header(&midi_header);

  track_headers = (track_header_t*) 
	malloc(midi_header.numberOfTracks*sizeof(track_header_t));

  for (i = 0; i< midi_header.numberOfTracks; i++ ) {
	read_track_header(track_headers + i);
	track_headers[i].trackID = i;
	print_track_header(track_headers +i);
	parse_event_data(track_headers + i);
  }

  fclose(midi_file);
  print_epilogue();
  return 0;
}
