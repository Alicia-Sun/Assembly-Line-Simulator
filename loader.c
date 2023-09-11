/*
 * loader.c : Defines loader functions for opening and loading object files
 */

#include "loader.h"
#include <stdio.h>
#include <stdlib.h>

unsigned short int swap_endian(unsigned short int num) {  //helper to swap little to big endian
    return ((num >> 8) & 0xFF) | ((num << 8) & 0xFF00);
}

// memory array location
unsigned short memoryAddress;

/*
 * Read an object file and modify the machine state as described in the writeup
 */
int ReadObjectFile(char* filename, MachineState* CPU) {

  // Opening the file
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) {
    fprintf(stderr, "Error opening file: %s\n", filename);
    return -1; // Return an error code to indicate failure
  }

  unsigned short header, address, n;
  unsigned short instr, line, fileindex;
  fread(&header, 2, 1, fp);
  int numread = -1;
  while (numread != 0) {
    header = swap_endian(header);	


    //now we compare the header with the different sections
    //remember to swap endian
    if (header == 0xCADE){
      //code
      fread(&address, 2, 1, fp);
      address = swap_endian(address);
      fread(&n, 2, 1, fp);
      n = swap_endian(n);
      for (int i = 0; i < n; i++) {
        fread(&instr, 2, 1, fp);
        instr = swap_endian(instr);
        CPU->memory[address + i] = instr;
      }
    } else if (header == 0xDADA) {
      //data
      fread(&address, 2, 1, fp);
      address = swap_endian(address);
      fread(&n, 2, 1, fp);
      n = swap_endian(n);

      for (int i = 0; i < n; i++) {
        fread(&instr, 2, 1, fp);
        instr = swap_endian(instr);
        CPU->memory[address + i] = instr;
      }
    } else if (header == 0xC3B7) {
      //symbol
      fread(&address, 2, 1, fp);
      address = swap_endian(address);
      fread(&n, 2, 1, fp);
      n = swap_endian(n);

      for (int i = 0; i < n; i++) {
        // do nothing
        fread(&instr, 1, 1, fp);
      }
    } else if (header == 0xF17E) {
      
      fread(&n, 1, 1, fp);

      for (int i = 0; i < n; i++) {
        //do nothing
      }
    } else if (header == 0x715E) {
      fread(&address, 2, 1, fp);
      address = swap_endian(address);
      fread(&line, 2, 1, fp);
      line = swap_endian(line);
      fread(&fileindex, 2, 1, fp);
      fileindex = swap_endian(fileindex);
    } else {
      fclose(fp);
      printf("Not formatted correctly\n");
      return -1;
    }		
    numread = fread(&header, 2, 1, fp);
  }
  //closing and return
	fclose(fp);
  return 0;
}
