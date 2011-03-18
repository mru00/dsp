
#include "common.h"

int main(int argc, char** argv) {

  if (argc != 2) return 1;
  char buffer[80];

  FILE* file = fopen(argv[1], "rb");

  if ( file == NULL ) return 2;

  fread(buffer, 4, 1, file);
  if (strcmp(buffer, "RIFF") != 0) return 3;
  fseek(file, 4, SEEK_CUR);
  fseek(file, 4, SEEK_CUR);

/*   Offset	Size	Description	Value */
/* 0x00	4	Chunk ID	"RIFF" (0x52494646) */
/* 0x04	4	Chunk Data Size	(file size) - 8 */
/* 0x08	4	RIFF Type	"WAVE" (0x57415645) */
/* 0x10	Wave chunks */



  fread(buffer, 4, 1, file);
  if (strcmp(buffer, "fmt ") != 0) return 4;
  fseek(file, 4, SEEK_CUR);
  fseek(file, 2, SEEK_CUR);
  fseek(file, 2, SEEK_CUR);


/* Offset	Size	Description	Value */
/* 0x00	4	Chunk ID	"fmt " (0x666D7420) */
/* 0x04	4	Chunk Data Size	16 + extra format bytes */
/* 0x08	2	Compression code	1 - 65,535 */
/* 0x0a	2	Number of channels	1 - 65,535 */
/* 0x0c	4	Sample rate	1 - 0xFFFFFFFF */
/* 0x10	4	Average bytes per second	1 - 0xFFFFFFFF */
/* 0x14	2	Block align	1 - 65,535 */
/* 0x16	2	Significant bits per sample	2 - 65,535 */
/* 0x18	2	Extra format bytes	0 - 65,535 */
/* 0x1a	Extra format bytes * */

  return 0;
}
