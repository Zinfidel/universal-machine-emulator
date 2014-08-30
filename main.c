#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

/* Eight indexable, general-purpose 32-bit registers. */
uint32_t Registers[8] = {0};

/* Pointers to program arrays. NULL pointer indicates an unallocated array.
 * Note that instructions retrieve the array pointers via their index in this
 * array! Effectively, the program array 0 is "referenced" by retrieving the
 * item at index 0.*/
uint32_t *Programs[NUM_ARRAYS] = {NULL};

/* Size (in 32-bit words) of the program arrays. */
size_t ProgramSize[NUM_ARRAYS] = {0};

/* Points to the current word to be read from a program array. */
uint32_t *ProgramCounter = NULL;


int main(int argc, char **argv) {
    // Take care of argument inspection, file loading, etc.
    if (Init(argc, argv) == RET_FAILURE) {
        return EXIT_FAILURE;
    }

    // Main loop
    for (;;) {
        // Read in the word the program counter points to, then advance.
        uint32_t word = *(ProgramCounter++);

        // Process the instruction
        Instruction inst = ParseInstruction(word);
        int retVal = RET_SUCCESS;
        switch(inst.opCode) {
            case CONDITIONAL_MOVE: ConditionalMove(inst);      break;
            case ARRAY_INDEX:      retVal = ArrayIndex(inst);  break;
            case ARRAY_UPDATE:     retVal = ArrayUpdate(inst); break;
            case ADDITION:         Add(inst);                  break;
            case MULTIPLICATION:   Multiply(inst);             break;
            case DIVISION:         retVal = Divide(inst);      break;
            case NAND:             Nand(inst);                 break;
            case HALT:             return(EXIT_SUCCESS);
            case ALLOCATION:       retVal = Allocate(inst);    break;
            case DEALLOCATION:     retVal = Deallocate(inst);  break;
            case OUTPUT:           retVal = Output(inst);      break;
            case INPUT:            retVal = Input(inst);       break;
            case LOAD_PROGRAM:     retVal = LoadProgram(inst); break;
            case LOAD_IMMEDIATE:   LoadImmediate(word);        break;
            default:               return(EXIT_FAILURE);
        }

        // Check for exit conditions, which include a failure return value from
        // the above functions, or the program counter pointing pointing
        // outside of the array.
        if (retVal == RET_FAILURE) {
            return EXIT_FAILURE;
        }
        if ((ProgramCounter - Programs[0]) >= ProgramSize[0]) {
            return EXIT_FAILURE;
        }
    }
}

/**
 * Initializes the 0 buffer and points the program counter to its first word.
 * @param argc Argument count.
 * @param argv Arguments.
 * @return RET_FAILURE if anything goes wrong, otherwise RET_SUCCESS.
 */
int Init(int argc, char **argv) {
    // Should only have one argument - the program to load.
    if (argc != 2) {
        PrintUsage(argv[0]);
        return RET_FAILURE;
    }

    // Try to load the file into array 0 and point the counter to it.
    LoadFile(argv[1], &Programs[0], &ProgramSize[0]);
    if (Programs[0] != NULL) {
        ProgramCounter = Programs[0];
        return RET_SUCCESS;
    } else {
        printf("Could not load %s.", argv[1]);
        PrintUsage(argv[0]);
        return RET_FAILURE;
    }
}

/**
 * Prints the command line usage.
 * 
 * @param programName The name of the program as it was called.
 */
void PrintUsage(char *programName) {
    printf("Usage: %s file\n", programName);
}

/**
 * Loads a program from a binary file into an array.
 * @param filePath The path to the binary file.
 * @param programArray Pointer to a program array. Memory will be allocated for
 * this pointer, and this pointer will be NULL if there are any errors.
 * @param size Gets set to the size (in 32-bit words) of the allocated array.
 */
void LoadFile(const char *filePath, uint32_t **programArray, size_t *size) {
    FILE *file = fopen(filePath, "rb");
    if (file == NULL) return;

    // Seek to the end of the file to get its size (in bytes) so that we can
    // allocate a suitably-sized array for the data. Rewind the stream after.
    fseek(file, 0L, SEEK_END);
    *size = ftell(file);
    *programArray = (uint32_t *)malloc(*size);
    rewind(file);

    // Finish up by reading the data into the program array. Endianess needs to
    // be converted, so one word is read at a time and converted.
    *size /= 4; // Convert size from bytes to 32-bit words.
    uint32_t buffer = 0, swapped = 0, i = 0;
    for (i; i < *size; i++) {
        fread(&buffer, sizeof(uint32_t), 1, file);
        swapped = ((buffer >> 24) & 0xff)      | // move byte 3 to byte 0
                  ((buffer << 8)  & 0xff0000)  | // move byte 1 to byte 2
                  ((buffer >> 8)  & 0xff00)    | // move byte 2 to byte 1
                  ((buffer << 24) & 0xff000000); // byte 0 to byte 3

        *(*programArray + i) = swapped;
    }
    fclose(file);
    return;
}

/**
 * Parses 32-bits into an Instruction bitfield.
 * 
 * @params instruction The 32 bits to parse.
 * @return Instruction Structure containing the parsed data.
 */
Instruction ParseInstruction(uint32_t instruction) {
    Instruction parsedInt;
    parsedInt.registerC = instruction & 7;
    parsedInt.registerB = (instruction >> 3) & 7;
    parsedInt.registerA = (instruction >> 6) & 7;
    parsedInt.opCode = (instruction >> 28) & 15;

    return parsedInt;
}

/**
 * The register A receives the value in register B, unless the register C is 0.
 * 
 * @param inst The Conditional Move instruction.
 */
void ConditionalMove(Instruction inst) {
    if (Registers[inst.registerC] != 0) {
        Registers[inst.registerA] = Registers[inst.registerB];
    }
}

/**
 * The register A receives the value stored at offset in register C in the
 * array identified by register B.
 * 
 * @param inst The Array Index instruction.
 * @return RET_FAILURE if anything goes wrong, RET_SUCCESS otherwise.
 */
int ArrayIndex(Instruction inst) {
  uint32_t *array = (uint32_t*) & Registers[inst.registerB];
  uint32_t offset = Registers[inst.registerC];
    
  // Referencing an unallocated array or accessing an out-of-bounds index is a
  // machine exception.
  if (array == NULL) {
    return RET_FAILURE;
  } 
  else {
    Registers[inst.registerA] = *(array + offset);
    return RET_SUCCESS;
  }
}

/**
 * The array identified by A is updated at the offset in register B to store the
 * value in register C.
 * 
 * @param inst The Array Update instruction.
 * @return RET_FAILURE if anything goes wrong, RET_SUCCESS otherwise.
 */
int ArrayUpdate(Instruction inst) {
  uint32_t *array = (uint32_t*) & Registers[inst.registerA];
  uint32_t offset = Registers[inst.registerB];

  // Referencing an unallocated array or accessing an out-of-bounds index is a
  // machine exception.
  if (array == NULL) {
    return RET_FAILURE;
  } else {
    *(array + offset) = Registers[inst.registerC];
    return RET_SUCCESS;
  }
}

/**
 * The register A receives the value in register B plus the value in register C,
 * modulo 2^32.
 * 
 * @param inst The Add instruction.
 */
void Add(Instruction inst) {
    Registers[inst.registerA] = Registers[inst.registerB] + Registers[inst.registerC];
}

/**
 * The register A receives the value in register B times the value in register
 * C, modulo 2^32.
 * 
 * @param inst The Multiply instruction.
 */
void Multiply(Instruction inst) {
    Registers[inst.registerA] = Registers[inst.registerB] * Registers[inst.registerC];
}

/**
 * The register A receives the value in register B divided by the value in
 * register C, where each quantity is treated as an unsigned 32-bit number.
 * 
 * @param inst The Divide instruction.
 * @return RET_FAILURE if anything goes wrong, RET_SUCCESS otherwise.
 */
int Divide(Instruction inst) {
    if (Registers[inst.registerC] != 0) {
        Registers[inst.registerA] = Registers[inst.registerB] / Registers[inst.registerC];
        return RET_SUCCESS;
    } else {
        return RET_FAILURE;
    }
}

/**
 * Each bit in the register A receives the 1 bit if either register B or
 * register C has a 0 bit in that position. Otherwise the bit in register A
 * receives the 0 bit.
 * 
 * @param inst The Nand instruction.
 */
void Nand(Instruction inst) {
    Registers[inst.registerA] = ~(Registers[inst.registerB] & Registers[inst.registerC]);
}

/**
 * A new array is created. The value in register C is the size in words of the
 * new array. The new array is zero-initialized. A reference index to the new
 * array is placed in register B.
 * 
 * @param inst The allocate instruction.
 * @return RET_FAILURE if anything goes wrong, RET_SUCCESS otherwise.
 */
int Allocate(Instruction inst) {
    // Do a linear scan through the array pointers to find one that is null.
    // The index of the first null pointer will be the new array "reference."
    // If we hit index NUM_ARRAYS, then there are no more arrays left!
    unsigned int index = 0; // Pre-increment skips program array 0.
    while ((++index < NUM_ARRAYS) && (Programs[index] != NULL));
    if (index == NUM_ARRAYS) return RET_FAILURE;

    ProgramSize[index] = Registers[inst.registerC];
    Programs[index] = (uint32_t *) calloc(ProgramSize[index], sizeof(uint32_t));
    Registers[inst.registerB] = index;

    return RET_SUCCESS;
}

/**
 * The array identified by register C is deallocated and freed for future use.
 * Machine exception thrown if an unallocated array is deallocated or if the 0
 * array is deallocated.
 * 
 * @param inst The Deallocate instruction.
 * @return RET_FAILURE if anything goes wrong, RET_SUCCESS otherwise.
 */
int Deallocate(Instruction inst) {
    unsigned int index = Registers[inst.registerC];
    if ((index == 0) || (index >= NUM_ARRAYS)) return RET_FAILURE;
    if (Programs[index] == NULL) return RET_FAILURE;

    // Free the memory, NULL the Programs array pointer, and clear the size.
    free(Programs[index]);
    Programs[index] = NULL;
    ProgramSize[index] = 0;
    return RET_SUCCESS;
}

/**
 * The value in the register C is displayed on the console. Only values in the
 * range 0-255 are allowed.
 * 
 * @param inst The Output instruction.
 * @return RET_FAILURE if anything goes wrong, RET_SUCCESS otherwise.
 */
int Output(Instruction inst) {
    uint32_t value = Registers[inst.registerC];
    if (value > 255) {
        return RET_FAILURE;
    } else {
        putchar(value);
        return RET_SUCCESS;
    }
}

/**
 * The machine waits for input on the console. When input arrives, the
 * register C is loaded with the input, which must be in the range of 0-255. If
 * the end of input has been signaled, then the register C is filled
 * with all 1's.
 * 
 * @param inst The Input instruction.
 * @return RET_FAILURE if anything goes wrong, RET_SUCCESS otherwise.
 */
int Input(Instruction inst) {
    int value = getchar();

    switch(value) {
        // End of file, or end of input (carriage return or ctrl + D)
        case EOF:
            Registers[inst.registerC] = -1;
            return RET_SUCCESS;

        default:
            if ((value > 255) || (value < 0)) {
                return RET_FAILURE;
            } else {
                Registers[inst.registerC] = value;
            }
            return RET_SUCCESS;
    }
}

/**
 * The array identified by the B register is duplicated and the duplicate
 * replaces the '0' array, regardless of size. The program counter is updated to
 * indicate the word of this array that is described by the offset given in C,
 * where the value 0 denotes the first word, 1 the second, etc.
 * 
 * @param inst The LoadProgram instruction.
 * @return RET_FAILURE if anything goes wrong, RET_SUCCESS otherwise.
 */
int LoadProgram(Instruction inst) {
    uint32_t index = Registers[inst.registerB];
    uint32_t offset = Registers[inst.registerC];
    uint32_t *program = Programs[index];
    size_t size = ProgramSize[index];

    // If the program is just using this instruction to move the program
    // counter, don't bother with copying memory and stuff.
    if (index == 0) {
        ProgramCounter = Programs[0] + offset;
        return RET_SUCCESS;
    }

    // Check for exceptions.
    if ((program == NULL) || (offset > size)) {
        return RET_FAILURE;
    }

    // Copy the specified array into array 0 and point to it.
    uint32_t *duplicate = (uint32_t *)malloc(size * sizeof(uint32_t));
    memcpy(duplicate, program, size * sizeof(uint32_t));
    free(Programs[0]);
    Programs[0] = duplicate;
    ProgramSize[0] = size;
    ProgramCounter = Programs[0] + offset;

    return RET_SUCCESS;
}

/**
 * The value in bits 0:24 is loaded into the register A (given by bits 25:27)
 * 
 * @param inst The LoadImmediate instruction, as an integer.
 */
void LoadImmediate(uint32_t inst) {
    // This uses a special instruction format.
    uint32_t value = inst & 0x01FFFFFF;
    unsigned int regA = (inst >> 25) & 7;
    Registers[regA] = value;
}
