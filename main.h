#include <stdint.h>
#ifndef MAIN_H
#define	MAIN_H

#define RET_SUCCESS 0
#define RET_FAILURE -1
#define NUM_ARRAYS 65536

/* Represents a standard 32-bit instruction. */
typedef struct
{
    unsigned registerA :3;
    unsigned registerB :3;
    unsigned registerC :3;
    unsigned           :19;
    unsigned opCode    :4;
} Instruction;

typedef struct
{ 
  uint32_t* array;
  unsigned int size;
} MemArray;

/* Represents all possible 4-bit instruction opcodes. */
typedef enum
{
    CONDITIONAL_MOVE = 0,
    ARRAY_INDEX      = 1,
    ARRAY_UPDATE     = 2,
    ADDITION         = 3,
    MULTIPLICATION   = 4,
    DIVISION         = 5,
    NAND             = 6,
    HALT             = 7,
    ALLOCATION       = 8,
    DEALLOCATION     = 9,
    OUTPUT           = 10,
    INPUT            = 11,
    LOAD_PROGRAM     = 12,
    LOAD_IMMEDIATE   = 13
} OpCode;

/* Function Prototypes */
int Init(int argc, char **argv);
void PrintUsage(char *programName);
void LoadFile(const char *filePath, MemArray* init_mem);
Instruction ParseInstruction(uint32_t instruction);
void ConditionalMove(Instruction inst);
int ArrayIndex(Instruction inst);
int ArrayUpdate(Instruction inst);
void Add(Instruction inst);
void Multiply(Instruction inst);
int Divide(Instruction inst);
void Nand(Instruction inst);
int Allocate(Instruction inst);
int Deallocate(Instruction inst);
int Output(Instruction inst);
int Input(Instruction inst);
int LoadProgram(Instruction inst);
void LoadImmediate(uint32_t inst);

#endif	/* MAIN_H */
