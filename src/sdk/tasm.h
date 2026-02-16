#ifndef TASM_H
#define TASM_H

// Core assembler function
// source: Null-terminated assembly source code
// outfile: Output filename for .texf
// Returns 0 on success, non-zero on failure
int tasm_assemble(char* source, char* outfile);

#endif
