/*
 * trace.c: location of main() to start the simulator
 */

#include "loader.h"

// Global variable defining the current state of the machine
MachineState* CPU;

int main(int argc, char** argv)
{
  MachineState machine;
  CPU = &machine;
  Reset(CPU);
  //checking arguments
  if (argc < 3) {
    fprintf(stderr, "You need more arguments.\n");
    return -1;
  }
  if (argv[1] == NULL) {
    fprintf(stderr, "Invalid filename\n");
    return -1;
  }
	
  for (int i = 2; i < argc; i++) {
    // Opening the file
    FILE* checkfile = fopen(argv[i], "rb");
    if (checkfile == NULL) {
      fprintf(stderr, "Invalid filename: %s\n", argv[i]);
      return -1; // Return an error code to indicate failure
    }
    fclose(checkfile);
    //reading to memory
    ReadObjectFile(argv[i], CPU);
  }

	

  FILE * tgtfile = fopen(argv[1], "w");      // open txt file to write to

  //loop through and update
  while((CPU->PC) != 0x80FF) {
      if (UpdateMachineState(CPU, tgtfile) != 0) {
      printf("There was an error!\n");
      return -1;
    }
	}

  fclose(tgtfile);

  printf("reached the end\n");
  return 0;
}
