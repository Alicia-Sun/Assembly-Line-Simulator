/*
 * LC4.c: Defines simulator functions for executing instructions
 */

#include "LC4.h"
#include <stdio.h>


#define INSN_15(I) ((I >> 15) & 0x1)                // [15]
#define INSN_15_12(I) ((I >> 12) & 0xF)             // [15:12]
#define INSN_11(I) ((I >> 11) & 0x1)                // [11]
#define INSN_11_9(I) ((I >> 9) & 0x7)               // [11:9]
#define INSN_8(I) ((I >> 8) & 0x1)                  // [8]
#define INSN_8_7(I) ((I >> 7) & 0x3)                // [8:7]
#define INSN_8_6(I) ((I >> 6) & 0x7)                // [8:6]
#define INSN_6(I) ((I >> 6) & 0x1)                  // [6]
#define INSN_5(I) ((I >> 5) & 0x1)                  // [5]
#define INSN_5_4(I) ((I >> 4) & 0x3)                // [5:4]
#define INSN_5_3(I) ((I >> 3) & 0x7)                // [5:3]
#define INSN_2_0(I) (I & 0x7)                       // [2:0]
#define IMM11(I) ((I & 0x7FF) | ((I & 0x400) ? 0xF800 : 0))     // last 11
#define IMM9(I) ((I & 0x1FF) | ((I & 0x100) ? 0xFE00 : 0))      // last 9
#define IMM8(I) (I & 0xFF)                                      // last 8
#define IMM7(I) ((I & 0x7F) | ((I & 0x40) ? 0xFF00 : 0))        // last 7
#define IMM6(I) ((I & 0x3F) | ((I & 0x20) ? 0xFFC0 : 0))        // last 6
#define IMM5(I) ((I & 0x1F) | ((I & 0x10) ? 0xFFE0 : 0))        // last 5

signed short signext11 (signed short number) {
  signed short ret = number & 0b0000011111111111;
  // Checking if sign-bit of number is 0 or 1
  if (number & 0b10000000000) {
    ret = ret | 0b1111100000000000;
  }
  return ret;
}
signed short signext9 (signed short number) {
  signed short ret = number & 0b0000000111111111;
  // Checking if sign-bit of number is 0 or 1
  if (number & 0b100000000) {
    ret = ret | 0b1111111000000000;
  }
  return ret;
}
signed short signext7 (signed short number) {
  signed short ret = number & 0b0000000001111111;
  // Checking if sign-bit of number is 0 or 1
  if (number & 0b1000000) {
    ret = ret | 0b1111111110000000;
  }
  return ret;
}
signed short signext6 (signed short number) {
  signed short ret = number & 0b0000000000111111;
  // Checking if sign-bit of number is 0 or 1
  if (number & 0b100000) {
    ret = ret | 0b1111111111000000;
  }
  return ret;
}
signed short signext5 (signed short number) {
  signed short ret = number & 0b0000000000011111;
  // Checking if sign-bit of number is 0 or 1
  if (number & 0b10000) {
    ret = ret | 0b1111111111100000;
  }
  return ret;
}


// Function to convert an unsigned short int to its binary equivalent
char* bin(unsigned short int value) {
  static char binary[17]; // 16 bits + '\0' null terminator
  int index = 0;

  do {
    binary[index++] = (value % 2) + '0'; // Get the remainder and convert it to '0' or '1'
    value /= 2;
  } while (value > 0);

  // Add leading zeros if needed to make it 16 bits
  while (index < 16) {
    binary[index++] = '0';
  }

  // Reverse the binary string
  for (int i = 0, j = index - 1; i < j; i++, j--) {
    char temp = binary[i];
    binary[i] = binary[j];
    binary[j] = temp;
  }

  binary[index] = '\0'; // Add null terminator

  return binary;
}

    
/*
 * Reset the machine state as Pennsim would do
 */
void Reset(MachineState* CPU)
{
  CPU->PC = 0x8200;               // default PC
  CPU->PSR = 0;
  ClearSignals(CPU);
	CPU->PSR |= 1 << 15;
  for (int i = 0; i < 8; i++) {           // reset registers
		CPU->R[i] = 0;
	}
  for (int i = 0; i < 65536; i++) {       // reset memory
		CPU->memory[i] = 0;
	}
  CPU->NZPVal = 0;
  CPU->dmemAddr = 0;
  CPU->dmemValue = 0;
}


/*
 * Clear all of the control signals (set to 0)
 */
void ClearSignals(MachineState* CPU)
{
  // Clear all control signals
  CPU->rsMux_CTL = 0;
  CPU->rtMux_CTL = 0;
  CPU->rdMux_CTL = 0;
  CPU->regFile_WE = 0;
  CPU->NZP_WE = 0;
  CPU->DATA_WE = 0;
}


/*
 * This function should write out the current state of the CPU to the file output.
 */
void WriteOut(MachineState* CPU, FILE* output)
{
  // Write the current state of the CPU to the output file
  fprintf(output, "PC: 0x%.4hX PSR: 0x%.4hX\n", CPU->PC, CPU->PSR);
  fprintf(output, "R0: 0x%.4hX R1: 0x%.4hX R2: 0x%.4hX R3: 0x%.4hX R4: 0x%.4hX R5: 0x%.4hX R6: 0x%.4hX R7: 0x%.4hX\n",
    CPU->R[0], CPU->R[1], CPU->R[2], CPU->R[3], CPU->R[4], CPU->R[5], CPU->R[6], CPU->R[7]);
  // does not write out control signals
  fprintf(output, "regIn: 0x%.4hX NZP: 0x%.4hX dmemAddr: 0x%.4hX dmemValue: 0x%.4hX\n",
    CPU->regInputVal, CPU->NZPVal, CPU->dmemAddr, CPU->dmemValue);

}


/*
 * This function should execute one LC4 datapath cycle.
 */
int UpdateMachineState(MachineState* CPU, FILE* output)
{
  //check if in data section
  if (CPU->PC >= 0x2000 && CPU->PC <= 0x7FFF) {
    printf("You're in a data section!\n");
    return -1;
  }
  if (CPU->PC >= 0xA000) {
    printf("You're in the OS data section!\n");
    return -1;
  }

  //now check if privilege matches section
  unsigned short privilege = INSN_15(CPU->PSR);
  if (privilege == 0 && CPU->PC >= 0x8000) {
    printf("User privilege trying to enter OS!\n");
    return -1;
  }
  if (privilege == 1 && CPU->PC <= 0x1FFF) {
    printf("OS privilege trying to enter User!\n");
    return -1;
  }

  unsigned short instruction = CPU->memory[CPU->PC];
  unsigned short opcode = INSN_15_12(instruction);
  if (opcode == 0) {
    // BranchOp
    BranchOp(CPU, output);

  } else if (opcode == 1) {
    ArithmeticOp(CPU, output);

  } else if (opcode == 2) {
    ComparativeOp(CPU, output);

  } else if (opcode == 4) {
    JSROp(CPU, output);
  } else if (opcode == 5) {
    LogicalOp(CPU, output);
  } else if (opcode == 10) {
    ShiftModOp(CPU, output);
  } else if (opcode == 12) {
    JumpOp(CPU, output);
  } else if (opcode == 6) {

    // LDR
    unsigned short privilege = INSN_15(CPU->PSR);
    unsigned short rd = INSN_11_9(instruction);
    unsigned short rs = INSN_8_6(instruction);
    signed short offset = IMM6(instruction);
    
    // check if privilege matches
    if (privilege == 0) {
      if (CPU->R[rs] + signext6(offset) >= 0xA000) {
        printf("trying to ldr into OS memory with wrong privilege");
        return -1;
      }
    } else {
      if (CPU->R[rs] + signext6(offset) <= 0x7FFF) {
        printf("trying to ldr into User memory with wrong privilege");
        return -1;
      }
    }
    CPU->dmemAddr = CPU->R[rs] + signext6(offset);

    fprintf(output, "%04X ", CPU->PC);
    fprintf(output, "%s ", bin(instruction));
    
    // control signals
    CPU->R[rd] = CPU->memory[CPU->R[rs] + signext6(offset)];
	  CPU->PC = CPU->PC + 1;
    SetNZP(CPU, CPU->R[rd]);

    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    
    CPU->dmemValue = CPU->R[rd];
    

    // printing copy pasted
    fprintf(output, "%u ", (unsigned int) CPU->regFile_WE);
    if ((unsigned int) CPU->regFile_WE == 1) { // if regfileWE set to high
      unsigned short int tgtregister = INSN_11_9(instruction);
      fprintf(output, "%X ", tgtregister);
      fprintf(output, "%04X ", CPU->R[tgtregister]);
    } else {
        fprintf(output, "0 0000 ");
    }
    fprintf(output, "%u ", CPU->NZP_WE);         // the NZP WE
    if (CPU->NZP_WE == 1) {                     // if NZP WE is high
      fprintf(output, "%X ", CPU->NZPVal);
    } else {
      fprintf(output, "0 ");
    }
    fprintf(output, "%u ", (unsigned int) CPU->DATA_WE);
    fprintf(output, "%04X %04X\n", (unsigned int) CPU->dmemAddr, (unsigned int) CPU->dmemValue);

  } else if (opcode == 7) {
    // STR
    unsigned short privilege = INSN_15(CPU->PSR);
    unsigned short rt = INSN_11_9(instruction);
    unsigned short rs = INSN_8_6(instruction);
    signed short offset = IMM6(instruction);

    //checking valid privilege
    if (privilege == 0) {
      if (CPU->R[rs] + signext6(offset) >= 0xA000) {
        printf("trying to str into OS memory with wrong privilege");
        return -1;
      }
    } else {
      if (CPU->R[rs] + signext6(offset) <= 0x7FFF) {
        printf("trying to str into User memory with wrong privilege");
        return -1;
      }
    }

    fprintf(output, "%04X ", CPU->PC);
    fprintf(output, "%s ", bin(instruction));
    
    //control signals
    CPU->memory[CPU->R[rs] + signext6(offset)] = CPU->R[rt];
    CPU->PC = CPU->PC + 1; 
    SetNZP(CPU, (signed short) CPU->R[rt]);

    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 1;
    CPU->rdMux_CTL = 0;
    
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 1;
    CPU->dmemAddr = CPU->R[rs] + signext6(offset);
    CPU->dmemValue = CPU->R[rt];

    // printing
    fprintf(output, "%u ", (unsigned int) CPU->regFile_WE);
    if ((unsigned int) CPU->regFile_WE == 1) { // if regfileWE set to high
      unsigned short int tgtregister = INSN_11_9(instruction);
      fprintf(output, "%X ", tgtregister);
      fprintf(output, "%04X ", CPU->R[tgtregister]);
    } else {
        fprintf(output, "0 0000 ");
    }
    fprintf(output, "%u ", CPU->NZP_WE);         // the NZP WE
    if (CPU->NZP_WE == 1) {                     // if NZP WE is high
      fprintf(output, "%X ", CPU->NZPVal);
    } else {
      fprintf(output, "0 ");
    }
    fprintf(output, "%u ", (unsigned int) CPU->DATA_WE);
    fprintf(output, "%04X %04X\n", (unsigned int) CPU->dmemAddr, (unsigned int) CPU->dmemValue);

    

  } else if (opcode == 8) {
    // RTI
    fprintf(output, "%04X ", CPU->PC);
    fprintf(output, "%s ", bin(instruction));
    CPU->PC = (unsigned short) CPU->R[7];
    CPU->PSR = CPU->PSR & 0b0111111111111111;

    //control signals
    CPU->rsMux_CTL = 1;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

    // printing copy pasted
    fprintf(output, "%u ", (unsigned int) CPU->regFile_WE);
    if ((unsigned int) CPU->regFile_WE == 1) { // if regfileWE set to high
      unsigned short int tgtregister = INSN_11_9(instruction);
      fprintf(output, "%X ", tgtregister);
      fprintf(output, "%04X ", CPU->R[tgtregister]);
    } else {
        fprintf(output, "0 0000 ");
    }
    fprintf(output, "%u ", CPU->NZP_WE);         // the NZP WE
    if (CPU->NZP_WE == 1) {                     // if NZP WE is high
      fprintf(output, "%X ", CPU->NZPVal);
    } else {
      fprintf(output, "0 ");
    }
    fprintf(output, "%u ", (unsigned int) CPU->DATA_WE);
    fprintf(output, "%04X %04X\n", (unsigned int) CPU->dmemAddr, (unsigned int) CPU->dmemValue);

  } else if (opcode == 9) {
    // CONST
    fprintf(output, "%04X ", CPU->PC);
    fprintf(output, "%s ", bin(instruction));
    unsigned short rd = INSN_11_9(instruction);
    signed short offset = ((instruction) & 0x01FF);
    CPU->R[rd] = signext9(offset);
    CPU->PC = CPU->PC + 1; 
    SetNZP(CPU, CPU->R[rd]);

    //control signals
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

    // printing
    fprintf(output, "%u ", (unsigned int) CPU->regFile_WE);
    if ((unsigned int) CPU->regFile_WE == 1) { // if regfileWE set to high
      unsigned short int tgtregister = INSN_11_9(instruction);
      fprintf(output, "%X ", tgtregister);
      fprintf(output, "%04X ", CPU->R[tgtregister]);
    } else {
        fprintf(output, "0 0000 ");
    }
    fprintf(output, "%u ", CPU->NZP_WE);         // the NZP WE
    if (CPU->NZP_WE == 1) {                     // if NZP WE is high
      fprintf(output, "%X ", CPU->NZPVal);
    } else {
      fprintf(output, "0 ");
    }
    fprintf(output, "%u ", (unsigned int) CPU->DATA_WE);
    fprintf(output, "%04X %04X\n", (unsigned int) CPU->dmemAddr, (unsigned int) CPU->dmemValue);

  } else if (opcode == 13) {
    // HICONST
    fprintf(output, "%04X ", CPU->PC);
    fprintf(output, "%s ", bin(instruction));
    unsigned short rd = INSN_11_9(instruction);
    unsigned short offset = (instruction) & 0x00FF;
    CPU->R[rd] = (((short)CPU->R[rd]) & 0x00FF) | (offset << 8);
    CPU->PC = CPU->PC + 1; 
    SetNZP(CPU, CPU->R[rd]);

    CPU->rsMux_CTL = 2;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

    // printing
    fprintf(output, "%u ", (unsigned int) CPU->regFile_WE);
    if ((unsigned int) CPU->regFile_WE == 1) { // if regfileWE set to high
      unsigned short int tgtregister = INSN_11_9(instruction);
      fprintf(output, "%X ", tgtregister);
      fprintf(output, "%04X ", CPU->R[tgtregister]);
    } else {
        fprintf(output, "0 0000 ");
    }
    fprintf(output, "%u ", CPU->NZP_WE);         // the NZP WE
    if (CPU->NZP_WE == 1) {                     // if NZP WE is high
      fprintf(output, "%X ", CPU->NZPVal);
    } else {
      fprintf(output, "0 ");
    }
    fprintf(output, "%u ", (unsigned int) CPU->DATA_WE);
    fprintf(output, "%04X %04X\n", (unsigned int) CPU->dmemAddr, (unsigned int) CPU->dmemValue);

  } else if (opcode == 15) {
    // TRAP
    fprintf(output, "%04X ", CPU->PC);
    fprintf(output, "%s ", bin(instruction));
    CPU->R[7] = CPU->PC + 1;
    unsigned short offset = (instruction & 0x00FF);
    CPU->PC = (0x8000 | offset);
    CPU->PSR = CPU->PSR | 0b1000000000000000;
    SetNZP(CPU, CPU->R[7]);

    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 1;
    
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

    // printing
    fprintf(output, "%u ", (unsigned int) CPU->regFile_WE);
    if ((unsigned int) CPU->regFile_WE == 1) { // if regfileWE set to high
      unsigned short int tgtregister = 7;
      fprintf(output, "%X ", tgtregister);
      fprintf(output, "%04X ", CPU->R[tgtregister]);
    } else {
        fprintf(output, "0 0000 ");
    }
    fprintf(output, "%u ", CPU->NZP_WE);         // the NZP WE
    if (CPU->NZP_WE == 1) {                     // if NZP WE is high
      fprintf(output, "%X ", CPU->NZPVal);
    } else {
      fprintf(output, "0 ");
    }
    fprintf(output, "%u ", (unsigned int) CPU->DATA_WE);
    fprintf(output, "%04X %04X\n", (unsigned int) CPU->dmemAddr, (unsigned int) CPU->dmemValue);

  } else {
    printf("Cannot identify opcode!\n");
    return -1;
  }
  return 0;
}



//////////////// PARSING HELPER FUNCTIONS ///////////////////////////



/*
 * Parses rest of branch operation and updates state of machine.
 */
void BranchOp(MachineState* CPU, FILE* output)
{
  fprintf(output, "%04X ", CPU->PC);
  // Parse the instruction to get info
  unsigned short int instruction = CPU->memory[CPU->PC];
  fprintf(output, "%s ", bin(instruction));
  signed short offset = IMM9(instruction);
  signed short extendedoffset = signext9(offset);
  unsigned short int condition = INSN_11_9(instruction);
  
  if ((CPU->NZPVal & condition) > 0) {
    // NZP val meets condition so check 
    CPU->PC = CPU->PC + 1 + extendedoffset;
  } else { // NZP val is different than required or NOP so just PC += 1
    CPU->PC = CPU->PC + 1;
  }
  CPU->rsMux_CTL = 0;
  CPU->rtMux_CTL = 0;
  CPU->rdMux_CTL = 0;
  
  CPU->regFile_WE = 0;
  CPU->NZP_WE = 0;
  CPU->DATA_WE = 0;
  CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

  // printing
  fprintf(output, "%u ", (unsigned int) CPU->regFile_WE);
  if ((unsigned int) CPU->regFile_WE == 1) { // if regfileWE set to high
    unsigned short int tgtregister = INSN_11_9(instruction);
    fprintf(output, "%X ", tgtregister);
    fprintf(output, "%04X ", CPU->R[tgtregister]);
  } else {
      fprintf(output, "0 0000 ");
  }
  fprintf(output, "%u ", CPU->NZP_WE);         // the NZP WE
  if (CPU->NZP_WE == 1) {                     // if NZP WE is high
    fprintf(output, "%X ", CPU->NZPVal);
  } else {
    fprintf(output, "0 ");
  }
  fprintf(output, "%u ", (unsigned int) CPU->DATA_WE);
  fprintf(output, "%04X %04X\n", (unsigned int) CPU->dmemAddr, (unsigned int) CPU->dmemValue);
}

/*
 * Parses rest of arithmetic operation and prints out.
 */
void ArithmeticOp(MachineState* CPU, FILE* output)
{
  fprintf(output, "%04X ", CPU->PC);
  // Parse the instruction to get info
  unsigned short int instruction = CPU->memory[CPU->PC];
  fprintf(output, "%s ", bin(instruction));
  if (INSN_5(instruction) == 1) {
    // this is immediate addition
    unsigned short rd = INSN_11_9(instruction);
    unsigned short rs = INSN_8_6(instruction);
    signed short offset = (signed short) IMM5(instruction);
    CPU->R[rd] = ((signed short)CPU->R[rs]) + signext5(offset);
    CPU->PC = CPU->PC + 1;
    if ((signed short) CPU->R[rd] < 0) {
      CPU->NZPVal = 4;
    } else if ((signed short) CPU->R[rd] == 0) {
      CPU->NZPVal = 2;
    } else if ((signed short) CPU->R[rd] > 0) {
      CPU->NZPVal = 1;
    } else {
      printf("NZP Error");
    }
  } else {
    unsigned short int operator = INSN_5_3(instruction);
    unsigned short rd = INSN_11_9(instruction);
    unsigned short rs = INSN_8_6(instruction);
    unsigned short rt = INSN_2_0(instruction);
    if (operator == 0) {
      // addition
      CPU->R[rd] = (signed short)CPU->R[rs] + (signed short)CPU->R[rt];
      CPU->PC = CPU->PC + 1;
      if ((signed short) CPU->R[rd] < 0) {
        CPU->NZPVal = 4;
      } else if ((signed short) CPU->R[rd] == 0) {
        CPU->NZPVal = 2;
      } else if ((signed short) CPU->R[rd] > 0) {
        CPU->NZPVal = 1;
      } else {
        printf("NZP Error");
      }
    } else if (operator == 1) {
      // multiplication
      CPU->R[rd] = (signed short)CPU->R[rs] * (signed short)CPU->R[rt];
      CPU->PC = CPU->PC + 1;
      if ((signed short)CPU->R[rd] < 0) {
        CPU->NZPVal = 4;
      } else if ((signed short)CPU->R[rd] == 0) {
        CPU->NZPVal = 2;
      } else if ((signed short)CPU->R[rd] > 0) {
        CPU->NZPVal = 1;
      } else {
        printf("NZP Error");
      }
    } else if (operator == 2) {
      // subtraction
      CPU->R[rd] = (signed short)CPU->R[rs] - (signed short)CPU->R[rt];
      CPU->PC = CPU->PC + 1;
      if ((signed short)CPU->R[rd] < 0) {
        CPU->NZPVal = 4;
      } else if ((signed short)CPU->R[rd] == 0) {
        CPU->NZPVal = 2;
      } else if ((signed short)CPU->R[rd] > 0) {
        CPU->NZPVal = 1;
      } else {
        printf("NZP Error");
      }
    } else if (operator == 3) {
      // division
      CPU->R[rd] = ((signed short) CPU->R[rs] / (signed short) CPU->R[rt]);
      CPU->PC = CPU->PC + 1;
      if ((signed short)CPU->R[rd] < 0) {
        CPU->NZPVal = 4;
      } else if ((signed short)CPU->R[rd] == 0) {
        CPU->NZPVal = 2;
      } else if ((signed short)CPU->R[rd] > 0) {
        CPU->NZPVal = 1;
      } else {
        printf("NZP Error");
      }
    } else {
      printf("Incorrectly formatted instructions!\n");
    }
  }
  //update control signals
  CPU->rsMux_CTL = 0;
  CPU->rtMux_CTL = 0;
  CPU->rdMux_CTL = 0;
  
  CPU->regFile_WE = 1;
  CPU->NZP_WE = 1;
  CPU->DATA_WE = 0;
  CPU->dmemAddr = 0;
  CPU->dmemValue = 0;

  // printing
  fprintf(output, "%u ", (unsigned int) CPU->regFile_WE);
  if ((unsigned int) CPU->regFile_WE == 1) { // if regfileWE set to high
    unsigned short int tgtregister = INSN_11_9(instruction);
    fprintf(output, "%X ", tgtregister);
    fprintf(output, "%04X ", CPU->R[tgtregister]);
  } else {
    fprintf(output, "0 0000 ");
  }
  fprintf(output, "%u ", CPU->NZP_WE);         // the NZP WE
  if (CPU->NZP_WE == 1) {                     // if NZP WE is high
    fprintf(output, "%X ", CPU->NZPVal);
  } else {
    fprintf(output, "0 ");
  }
  fprintf(output, "%u ", (unsigned int) CPU->DATA_WE);
  fprintf(output, "%04X %04X\n", (unsigned int) CPU->dmemAddr, (unsigned int) CPU->dmemValue);
}

/*
 * Parses rest of comparative operation and prints out.
 */
void ComparativeOp(MachineState* CPU, FILE* output)
{
  fprintf(output, "%04X ", CPU->PC);
  // Parse the instruction to get info
  unsigned short int instruction = CPU->memory[CPU->PC];
  fprintf(output, "%s ", bin(instruction));
  unsigned short int cmptype = INSN_8_7(instruction);
  
  // go through different compare types by code
  unsigned short rs = INSN_11_9(instruction);
  if (cmptype == 2) {
    signed short imm7 = IMM7(instruction);
    signed short diff = ((signed short)CPU->R[rs] - signext7(imm7));
    //update NZP
    if (diff < 0) {
      CPU->NZPVal = 4;
    } else if (diff == 0) {
      CPU->NZPVal = 2;
    } else if (diff > 0) {
      CPU->NZPVal = 1;
    } else {
      printf("NZP Error");
    }
  } else if (cmptype == 3) {
    unsigned short uimm7 = (instruction & 0x007F);
    signed short diff = (signed short)((unsigned short)CPU->R[rs] - uimm7);
    //update NZP
    if (diff < 0) {
      CPU->NZPVal = 4;
    } else if (diff == 0) {
      CPU->NZPVal = 2;
    } else if (diff > 0) {
      CPU->NZPVal = 1;
    } else {
      printf("NZP Error");
    }
  } else if (cmptype == 1) {
    unsigned short rt = INSN_2_0(instruction);
    signed short diff = (signed short)((unsigned short)CPU->R[rs] - (unsigned short)CPU->R[rt]);
    //update NZP
    if (diff < 0) {
      CPU->NZPVal = 4;
    } else if (diff == 0) {
      CPU->NZPVal = 2;
    } else if (diff > 0) {
      CPU->NZPVal = 1;
    } else {
      printf("NZP Error");
    }
  } else {
    unsigned short rt = INSN_2_0(instruction);
    signed short diff = (signed short)((signed short)CPU->R[rs] - (signed short)CPU->R[rt]);
    //update NZP
    if (diff < 0) {
      CPU->NZPVal = 4;
    } else if (diff == 0) {
      CPU->NZPVal = 2;
    } else if (diff > 0) {
      CPU->NZPVal = 1;
    } else {
      printf("NZP Error");
    }
  }
  CPU->PC = CPU->PC + 1;
  //update control signals
	CPU->rsMux_CTL = 2;
  CPU->rtMux_CTL = 0;
  CPU->rdMux_CTL = 0;
  
  CPU->regFile_WE = 0;
  CPU->NZP_WE = 1;
  CPU->DATA_WE = 0;
  CPU->dmemAddr = 0;
  CPU->dmemValue = 0;

  // printing
  fprintf(output, "%u ", (unsigned int) CPU->regFile_WE);
  if ((unsigned int) CPU->regFile_WE == 1) { // if regfileWE set to high
    unsigned short int tgtregister = INSN_11_9(instruction);
    fprintf(output, "%X ", tgtregister);
    fprintf(output, "%04X ", CPU->R[tgtregister]);
  } else {
    fprintf(output, "0 0000 ");
  }
  fprintf(output, "%u ", CPU->NZP_WE);         // the NZP WE
  if (CPU->NZP_WE == 1) {                     // if NZP WE is high
    fprintf(output, "%X ", CPU->NZPVal);
  } else {
    fprintf(output, "0 ");
  }
  fprintf(output, "%u ", (unsigned int) CPU->DATA_WE);
  fprintf(output, "%04X %04X\n", (unsigned int) CPU->dmemAddr, (unsigned int) CPU->dmemValue);
  
}

/*
 * Parses rest of logical operation and prints out.
 */
void LogicalOp(MachineState* CPU, FILE* output)
{
  fprintf(output, "%04X ", CPU->PC);
  // Parse the instruction to get info
  unsigned short int instruction = CPU->memory[CPU->PC];
  fprintf(output, "%s ", bin(instruction));

  unsigned short rd = INSN_11_9(instruction);
  unsigned short rs = INSN_8_6(instruction);
  unsigned short rt = INSN_2_0(instruction);

  if (INSN_5(instruction) == 1) {
    // this is immediate AND
    unsigned short rd = INSN_11_9(instruction);
    unsigned short rs = INSN_8_6(instruction);
    signed short offset = IMM5(instruction);
    CPU->R[rd] = CPU->R[rs] & signext5(offset);
    CPU->PC = CPU->PC + 1;
    //update NZP
    if (CPU->R[rd] < 0) {
      CPU->NZPVal = 4;
    } else if (CPU->R[rd] == 0) {
      CPU->NZPVal = 2;
    } else if (CPU->R[rd] > 0) {
      CPU->NZPVal = 1;
    } else {
      printf("NZP Error");
    }
  } else {
    unsigned short int operator = INSN_5_3(instruction);
    unsigned short rd = INSN_11_9(instruction);
    unsigned short rs = INSN_8_6(instruction);
    unsigned short rt = INSN_2_0(instruction);
    if (operator == 0) {
      // AND
      CPU->R[rd] = CPU->R[rs] & CPU->R[rt];
      CPU->PC = CPU->PC + 1;
      //update NZP
      if ((signed short)CPU->R[rd] < 0) {
        CPU->NZPVal = 4;
      } else if ((signed short)CPU->R[rd] == 0) {
        CPU->NZPVal = 2;
      } else if ((signed short)CPU->R[rd] > 0) {
        CPU->NZPVal = 1;
      } else {
        printf("NZP Error");
      }
    } else if (operator == 1) {
      // NOT
      CPU->R[rd] = -1 * (CPU->R[rs]);
      CPU->PC = CPU->PC + 1;
      //update NZP
      if ((signed short)CPU->R[rd] < 0) {
        CPU->NZPVal = 4;
      } else if ((signed short)CPU->R[rd] == 0) {
        CPU->NZPVal = 2;
      } else if ((signed short)CPU->R[rd] > 0) {
        CPU->NZPVal = 1;
      } else {
        printf("NZP Error");
      }
    } else if (operator == 2) {
      // OR
      CPU->R[rd] = CPU->R[rs] | CPU->R[rt];
      CPU->PC = CPU->PC + 1;
      //update NZP
      if ((signed short)CPU->R[rd] < 0) {
        CPU->NZPVal = 4;
      } else if ((signed short)CPU->R[rd] == 0) {
        CPU->NZPVal = 2;
      } else if ((signed short)CPU->R[rd] > 0) {
        CPU->NZPVal = 1;
      } else {
        printf("NZP Error");
      }
    } else if (operator == 3) {
      // XOR
      CPU->R[rd] = ((short) CPU->R[rs] ^ (short) CPU->R[rt]);
      CPU->PC = CPU->PC + 1;
      //update NZP
      if ((signed short)CPU->R[rd] < 0) {
        CPU->NZPVal = 4;
      } else if ((signed short)CPU->R[rd] == 0) {
        CPU->NZPVal = 2;
      } else if ((signed short)CPU->R[rd] > 0) {
        CPU->NZPVal = 1;
      } else {
        printf("NZP Error");
      }
    } else {
      printf("Incorrectly formatted instructions!\n");
    }
  }
  //update controls
  CPU->rsMux_CTL = 0;
  CPU->rtMux_CTL = 0;
  CPU->rdMux_CTL = 0;
  
  CPU->regFile_WE = 1;
  CPU->NZP_WE = 1;
  CPU->DATA_WE = 0;
  CPU->dmemAddr = 0;
  CPU->dmemValue = 0;
  
  // printing is copy pasted
  fprintf(output, "%u ", (unsigned int) CPU->regFile_WE);
  if ((unsigned int) CPU->regFile_WE == 1) { // if regfileWE set to high
    unsigned short int tgtregister = INSN_11_9(instruction);
    fprintf(output, "%X ", tgtregister);
    fprintf(output, "%04X ", CPU->R[tgtregister]);
  } else {
    fprintf(output, "0 0000 ");
  }
  fprintf(output, "%u ", CPU->NZP_WE);         // the NZP WE
  if (CPU->NZP_WE == 1) {                     // if NZP WE is high
    fprintf(output, "%X ", CPU->NZPVal);
  } else {
    fprintf(output, "0 ");
  }
  fprintf(output, "%u ", (unsigned int) CPU->DATA_WE);
  fprintf(output, "%04X %04X\n", (unsigned int) CPU->dmemAddr, (unsigned int) CPU->dmemValue);
}

/*
 * Parses rest of jump operation and prints out.
 */
void JumpOp(MachineState* CPU, FILE* output)
{
  fprintf(output, "%04X ", CPU->PC);
  // Parse the instruction to get info
  unsigned short int instruction = CPU->memory[CPU->PC];
  fprintf(output, "%s ", bin(instruction));

  if (INSN_11(instruction) == 1) {
    // JMP IMM
    signed short offset = IMM11(instruction);
    CPU->PC = CPU->PC + 1 + signext11(offset);
  } else {
    // JMP
    unsigned short rs = INSN_8_6(instruction);
    CPU->PC = (unsigned short) CPU->R[rs];
  }

  //update controls
  CPU->rsMux_CTL = 0;
  CPU->rtMux_CTL = 0;
  CPU->rdMux_CTL = 0;
  
  CPU->regFile_WE = 0;
  CPU->NZP_WE = 0;
  CPU->DATA_WE = 0;
  CPU->dmemAddr = 0;
  CPU->dmemValue = 0;
  
  // printing  is copy pasted
  fprintf(output, "%u ", (unsigned int) CPU->regFile_WE);
  if ((unsigned int) CPU->regFile_WE == 1) { // if regfileWE set to high
    unsigned short int tgtregister = INSN_11_9(instruction);
    fprintf(output, "%X ", tgtregister);
    fprintf(output, "%04X ", CPU->R[tgtregister]);
  } else {
    fprintf(output, "0 0000 ");
  }
  fprintf(output, "%u ", CPU->NZP_WE);         // the NZP WE
  if (CPU->NZP_WE == 1) {                     // if NZP WE is high
    fprintf(output, "%X ", CPU->NZPVal);
  } else {
    fprintf(output, "0 ");
  }
  fprintf(output, "%u ", (unsigned int) CPU->DATA_WE);
  fprintf(output, "%04X %04X\n", (unsigned int) CPU->dmemAddr, (unsigned int) CPU->dmemValue);
}

/*
 * Parses rest of JSR operation and prints out.
 */
void JSROp(MachineState* CPU, FILE* output)
{
  fprintf(output, "%04X ", CPU->PC);
  // Parse the instruction to get info
  unsigned short int instruction = CPU->memory[CPU->PC];
  fprintf(output, "%s ", bin(instruction));

  if (INSN_11(instruction) == 1) {
    // JSR
    signed short offset = IMM11(instruction);
    CPU->R[7]= CPU->PC + 1;
    if ((unsigned short)(CPU->R[7]) < 0) {
      CPU->NZPVal = 4;
    } else if ((unsigned short)(CPU->R[7]) == 0) {
      CPU->NZPVal = 2;
    } else if ((unsigned short)(CPU->R[7]) > 0) {
      CPU->NZPVal = 1;
    } else {
      printf("NZP Error");
    }
    CPU->PC = (CPU->PC & 0x8000) | (offset << 4);
  } else {
    // JSRR
    unsigned short rs = INSN_8_6(instruction);
    unsigned short int tempnext = CPU->PC + 1;
    CPU->PC = (unsigned short)(CPU->R[rs]);
    CPU->R[7]= tempnext;
    if ((unsigned short)(CPU->R[rs]) < 0) {
      CPU->NZPVal = 4;
    } else if ((unsigned short)(CPU->R[rs]) == 0) {
      CPU->NZPVal = 2;
    } else if ((unsigned short)(CPU->R[rs]) > 0) {
      CPU->NZPVal = 1;
    } else {
      printf("NZP Error");
    }
  }

  //update controls
  CPU->rsMux_CTL = 0;
  CPU->rtMux_CTL = 0;
  CPU->rdMux_CTL = 1;
  
  CPU->regFile_WE = 1;
  CPU->NZP_WE = 1;
  CPU->DATA_WE = 0;
  CPU->dmemAddr = 0;
  CPU->dmemValue = 0;
  
  // printing is copy pasted
  fprintf(output, "%u ", (unsigned int) CPU->regFile_WE);
  if ((unsigned int) CPU->regFile_WE == 1) { // if regfileWE set to high
    unsigned short int tgtregister = 7;
    fprintf(output, "%X ", tgtregister);
    fprintf(output, "%04X ", CPU->R[tgtregister]);
  } else {
    fprintf(output, "0 0000 ");
  }
  fprintf(output, "%u ", CPU->NZP_WE);         // the NZP WE
  if (CPU->NZP_WE == 1) {                     // if NZP WE is high
    fprintf(output, "%X ", CPU->NZPVal);
  } else {
    fprintf(output, "0 ");
  }
  fprintf(output, "%u ", (unsigned int) CPU->DATA_WE);
  fprintf(output, "%04X %04X\n", (unsigned int) CPU->dmemAddr, (unsigned int) CPU->dmemValue);
}

/*
 * Parses rest of shift/mod operations and prints out.
 */
void ShiftModOp(MachineState* CPU, FILE* output)
{
  
  fprintf(output, "%04X ", CPU->PC);
  // Parse the instruction to get info
  unsigned short int instruction = CPU->memory[CPU->PC];
  fprintf(output, "%s ", bin(instruction));
  unsigned short rd = INSN_11_9(instruction);
	unsigned short rs = INSN_8_6(instruction);
	
  if (INSN_5_4(instruction) == 3) {
    // MOD
    unsigned short rt = INSN_2_0(instruction);
    CPU->R[rd] = (signed short) ((signed short)CPU->R[rs]) % (signed short)(CPU->R[rt]);
    if ((signed short)(CPU->R[rd]) < 0) {
      CPU->NZPVal = 4;
    } else if ((signed short)(CPU->R[rd]) == 0) {
      CPU->NZPVal = 2;
    } else if ((signed short)(CPU->R[rd]) > 0) {
      CPU->NZPVal = 1;
    } else {
      printf("NZP Error");
    }
    CPU->PC = CPU->PC + 1;
  } else {
    if (INSN_5_4(instruction) == 0) {
      unsigned short offset = instruction & 0x000F;
      CPU->R[rd] = CPU->R[rs] << offset;
      if ((signed short)(CPU->R[rd]) < 0) {
        CPU->NZPVal = 4;
      } else if ((signed short)(CPU->R[rd]) == 0) {
        CPU->NZPVal = 2;
      } else if ((signed short)(CPU->R[rd]) > 0) {
        CPU->NZPVal = 1;
      } else {
        printf("NZP Error");
      }
      CPU->PC = CPU->PC + 1;
    } else if (INSN_5_4(instruction) == 1) {
      // SRA
      short int offset = instruction & 0x000F;


      CPU->R[rd] = ((short int)CPU->R[rs]);// >> offset;
      unsigned short msb = CPU->R[rs] >> 15;

      if (msb == 1) {
        //negative pad with 1s
        for (int i = 0; i < offset; i++) {
          CPU->R[rd] = CPU->R[rd] >> 1;
          CPU->R[rd] = CPU->R[rd] | 0b1000000000000000;
        }
      } else {
        CPU->R[rd] = ((signed short)CPU->R[rs]) >> offset;
      }
      
      if ((signed short)(CPU->R[rd]) < 0) {
        CPU->NZPVal = 4;
      } else if ((signed short)(CPU->R[rd]) == 0) {
        CPU->NZPVal = 2;
      } else if ((signed short)(CPU->R[rd]) > 0) {
        CPU->NZPVal = 1;
      } else {
        printf("NZP Error");
      }
      CPU->PC = CPU->PC + 1;
    } else if (INSN_5_4(instruction) == 2) {

      //SRL
      unsigned short offset = instruction & 0x000F;
      CPU->R[rd] = CPU->R[rs] >> offset;
      if ((signed short)(CPU->R[rd]) < 0) {
        CPU->NZPVal = 4;
      } else if ((signed short)(CPU->R[rd]) == 0) {
        CPU->NZPVal = 2;
      } else if ((signed short)(CPU->R[rd]) > 0) {
        CPU->NZPVal = 1;
      } else {
        printf("NZP Error");
      }
      CPU->PC = CPU->PC + 1;
    } else {
      //printf("instruction error!");
    }
  }
  CPU->rsMux_CTL = 0;
  CPU->rtMux_CTL = 0;
  CPU->rdMux_CTL = 0;
  
  CPU->regFile_WE = 1;
  CPU->NZP_WE = 1;
  CPU->DATA_WE = 0;
  CPU->dmemAddr = 0;
  CPU->dmemValue = 0;
  
  // printing
  fprintf(output, "%u ", (unsigned int) CPU->regFile_WE);
  if ((unsigned int) CPU->regFile_WE == 1) { // if regfileWE set to high
    unsigned short int tgtregister = INSN_11_9(instruction);
    fprintf(output, "%X ", tgtregister);
    fprintf(output, "%04X ", CPU->R[tgtregister]);
    
  } else {
    fprintf(output, "0 0000 ");
  }


  


  fprintf(output, "%u ", CPU->NZP_WE);         // the NZP WE
  if (CPU->NZP_WE == 1) {                     // if NZP WE is high
    fprintf(output, "%X ", CPU->NZPVal);
  } else {
    fprintf(output, "0 ");
  }
  fprintf(output, "%u ", (unsigned int) CPU->DATA_WE);
  fprintf(output, "%04X %04X\n", (unsigned int) CPU->dmemAddr, (unsigned int) CPU->dmemValue);
}

/*
 * Set the NZP bits in the PSR.
 */
void SetNZP(MachineState* CPU, signed short result)
{
  // negative is 4 zero is 2 positive is 1
  if (result < 0) {
    CPU->NZPVal = 4;
  } else if (result == 0) {
    CPU->NZPVal = 2;
  } else if (result > 0) {
    CPU->NZPVal = 1;
  } else {
    printf("NZP Error");
  }
}
