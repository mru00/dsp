#include <stdio.h>

int main() {

  float f;

  int i;

  do {
	i = scanf("%f", &f);
	printf("retval: %d\n", i);
  } while (i!=0);


  return 0;
}
