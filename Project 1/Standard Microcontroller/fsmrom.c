/*
 * CS2200 Fall 2008
 * Project 1, LC2200-16
 * FSM ROM Translator
 *
 * 13 Jan. 2005, Initial Revision, Kyle Goodwin
 * 21 Jan. 2005, Completed without CC, Kyle Goodwin
 * 24 Jan. 2005, Updated to include CC in dispatch, Kyle Goodwin
 * 30 Jan. 2005, Fixed some bugs with CC, Kyle Goodwin
 * 7  Jun. 2006, Updated error messages, Kane Bonnette
 * 30 Aug. 2006, Changed to 16-bit (rem LdIRLo/Hi), Kane Bonnette
 * 28 Aug. 2008, Added support for DOS and Macintosh files, Matt Bigelow
 */

/* 
Translator grammar:
   FSM        -> STATE FSM | STATE
   STATE      -> STATE_NAME:
                 STATE_DEF
                 goto STATE_NAME |
		 onz STATE_NAME else STATE_NAME |
		 dispatch
   STATE_NAME -> (an element from the states array below)
   STATE_DEF  -> SIGNAL STATE_DEF | SIGNAL
   SIGNAL     -> (an element from the control array below)
   
   Each state is specified in three lines (may have leading whitespace)...
   FETCH0:
       DrPC LdMAR
       goto FETCH1

   When you want to go to a next state based upon some other signal input
   besides the current state such as the opcode or the state of the Z
   register use the alternative transitions shown below:

   Go to BEQ3 if Z is set, BEQ2 otherwise:
   onz BEQ3 else BEQ2

   Choose the next state based upon the opcode input:
   dispatch

   For the special case of SPOP, the CC signal is also used to determine
   the proper state for a dispatch.

   This syntax uses the predefined base states for the various instructions
   to determine the next state.

   Each state follows directly after the previous one with no space between.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Uncomment the following line to enable verbose debugging output: */
/* #define DEBUG_VERBOSE */

/* The actual rom, 12 (word) address bits and 32 bits per word */
static unsigned int rom[(1 << 12) - 1];

/* Defines for the various input signal states */
#define Z_SET   (1 << 11)
#define CCL_SET (1 << 9)
#define CCH_SET (1 << 10)

/* Table of signals */

/* STATES (room for 6 bits, 64 possible states) */
const char *states[65] = {
  /*  0 */ "DUMMY",
  /*  1 */ "FETCH0", "FETCH1", "FETCH2", "FETCH3", "FETCH4",
           "FETCH5", "FETCH6", "FETCH7", "FETCH8", "FETCH9",
  /* 11 */ "DECODE",
  /* 12 */ "ADD0", "ADD1", "ADD2", "ADD3",
  /* 16 */ "ADDI0", "ADDI1", "ADDI2", "ADDI3",
  /* 20 */ "NAND0", "NAND1", "NAND2", "NAND3",
  /* 24 */ "LW0", "LW1", "LW2", "LW3",
  /* 28 */ "SW0", "SW1", "SW2", "SW3",
  /* 32 */ "BEQ0", "BEQ1", "BEQ2", "BEQ3", "BEQ4",
           "BEQ5", "BEQ6", "BEQ7", "BEQ8",
  /* 41 */ "JALR0", "JALR1", "JALR2", "JALR3", "JALR4",
  /* 46 */ "EI0", "EI1", "EI2", "EI3", "EI4",
  /* 51 */ "DI0", "DI1", "DI2", "DI3", "DI4",
  /* 56 */ "RETI0", "RETI1", "RETI2", "RETI3", "RETI4", "RETI5",
  /* 62 */ "RESERVED",
  /* 63 */ "HALT",
  NULL
};

#define DISPATCH_STATE 62

/* INITIAL STATES BY INSTRUCTION */
const unsigned int dispatchstate[9] = {
  /* opcode     */
  /*   000 ADD  */ 12,
  /*   001 NAND */ 20,
  /*   010 ADDI */ 16,
  /*   011 LW   */ 24,
  /*   100 SW   */ 28,
  /*   101 BEQ  */ 32,
  /*   110 JALR */ 41,
  /*   111 SPOP */ 46, /* DEPRECATED */
  /* TERMINATOR */  0
};

/* INITIAL STATES FOR SPOP BY CC VALUE */
const unsigned int spopstate[4] = {
        /* CC */
        /* 00 */ 63,
        /* 01 */ 46,
        /* 10 */ 51,
        /* 11 */ 56
};
        

#define OP_OFFSET 6
#define CC_OFFSET 9
#define CONTROL_BASE 6

/* CONTROL (start at CONTROL_BASE bits from 0, 1 bit per signal) */
const char *control[21] = {
  "DrREG",
  "DrMEM",
  "DrALU",
  "DrPC",
  "DrOFF",
  "LdPC",
  "LdIR",
  "LdMAR",
  "LdA",
  "LdB",
  "LdZ",
  "WrREG",
  "WrMEM",
  "SelPR",
  "RegSelLo",
  "RegSelHi",
  "ALULo",
  "ALUHi",
  /* "LdIE", */
  "HALT",
  NULL
};

/* Holds the line number of the line currently being parsed */
static unsigned int lineno = 0;

/* Sets the bit associated with the specified control signal */
void addcontrol(char *sig, unsigned int *output) {
        unsigned int i = 0;
        while (control[i] != NULL) {
                if (!strcmp(sig, control[i])) {
                        *output += 1 << (i + CONTROL_BASE);
                        return;
                }

                i++;
        }

        fprintf(stderr, "Line %d: Invalid control signal '%s'\n",
                lineno, sig);
        exit(1);
}

/* Parses a line (supposedly) containing a state header and returns the
   numerical value of the state */
unsigned int parsestate(char *line) {
        unsigned int i = 0;
        char *tmp;

        while (*line <= 40) line++;
        
        tmp = strchr(line, ':');
        
        if (tmp == NULL) {
                fprintf(stderr,
                        "Line %d: Malformed state header '%s' missing ':'\n",
                        lineno, line);
                exit(1);
        }

        *tmp = '\0';
        
        while (states[i] != NULL) {
                if (!strcmp(line, states[i]))
                        return i;
                i++;
        }

        fprintf(stderr, "Line %d: Invalid state name '%s'\n", lineno, line);
        exit(1);
}

/* Parses a line (supposedly) containing a state definition with multiple
   signals into several discrete signal strings which may then be parsed
   into their numerical equivilents to set the bits of the complete
   control signal specified by the state */
unsigned int parsesignals(char *line) {
        unsigned int signal = 0;
        char *cur;
	while (*line <= 40) line++;

        while (*(cur = strsep(&line, " ")) != '\0') {
	  addcontrol(cur, &signal);

#ifdef DEBUG_VERBOSE
	  printf("Signal: %X\n", signal);
#endif

	  if (line == NULL) break;
	}

        return signal;
}

/* Parses a line (supposedly) containing a state transition, optionally
   referencing the value of the z input or causing a dispatch to occur */
unsigned int parsetransition(char *line) {
  char *cur;
  unsigned int transition = 0xdeadbeef;
  unsigned int i = 0;

  while (*line <= 40) line++;

  if (!strncmp(line, "goto", 4)) {
    strsep(&line, " ");
    
    while (states[i] != NULL) {
      if (!strcmp(line, states[i])) {
	    /* the upper half of the transition specifies the next state
	       if z is set, the lower half if z is unset; these are both
	       the same in the case of a plain goto state tranition */
	    transition = (i << 16) + i;

	    return transition;
      }

      i++;
    }

    fprintf(stderr, "Line %d: Invalid state name in transition '%s'",
	    lineno, line);

    exit(1);
  }

  if (!strncmp(line, "onz", 3)) {
    strsep(&line, " ");
    cur = strsep(&line, " ");

    if (*cur == '\0') {
      fprintf(stderr, "Line %d: Invalid onz transition statement", lineno);

      exit(1);
    }

    while (states[i] != NULL) {
      if (!strcmp(cur, states[i])) {
	transition = (i << 16);
	break;
      }

      i++;
    }

    if (transition == 0xdeadbeef) {
      fprintf(stderr, "Line %d: Invalid state name in transition '%s'",
	      lineno, cur);

      exit(1);
    }

    cur = strsep(&line, " ");

    if (strcmp(cur, "else")) {
	fprintf(stderr, "Line %d: Invalid onz transition statement, missing else",
		lineno);

	exit(1);
    }

    i = 0;

    while (states[i] != NULL) {
      if (!strcmp(line, states[i])) {
	transition += i;
	return transition;
      }

      i++;
    }
    
    fprintf(stderr, "Line %d: Invalid state name in transition '%s'",
	    lineno, line);

    exit(1);
  }

  if (!strcmp(line, "dispatch")) {
    /* A special reserved state number is used for the dispatch
       transition, this state number is replaced by the corrent
       state number for the opcode specified at rom creation time */
    transition = DISPATCH_STATE;

    return transition;
  }
  
  fprintf(stderr, "Line %d: Expected state transition\n", lineno);
  
  exit(1);
}

/* Handles the parsing and rom update for one complete state
   for all values of opcode, z, and cc inputs */
unsigned int handlestate(FILE *fp) {
        unsigned int i, j, state, signals, transition;
  char t;
  char buf[256];

  lineno++;

  t = fgetc(fp);
  if((t == '\n') || (t == '\r'))
    while ((t == '\n') || (t == '\r'))
      t = fgetc(fp);

  fseek(fp, -1, SEEK_CUR);

  for (i = 0; i < 255; i++) {
    t = fgetc(fp);

    if ((t == '\n') || (t == '\r') || (t == EOF)) {
      if (i == 0) return 0;

      buf[i] = '\0';
      break;
    } else buf[i] = t;
  }

  if (i == 255) {
    fprintf(stderr, "Line %d: Line exceeded maximum length", lineno);
    
    exit(1);
  }

  state = parsestate(buf);

  lineno++;

  while ((t == '\n') || (t == '\r'))
    t = fgetc(fp);

  for (i = 0; i < 255; i++) {
    t = fgetc(fp);

    if ((t == '\n') || (t == '\r') || (t == EOF)) {
      buf[i] = '\0';
      break;
    } else buf[i] = t;
  }

  if (i == 255) {
    fprintf(stderr, "Line %d: Line exceeded maximum length", lineno);
    
    exit(1);
  }

  signals = parsesignals(buf);

  lineno++;

  while ((t == '\n') || (t == '\r'))
    t = fgetc(fp);

  for (i = 0; i < 255; i++) {
    t = fgetc(fp);

    if ((t == '\n') || (t == '\r') || (t == EOF)) {
      buf[i] = '\0';
      break;
    } else buf[i] = t;
  }

  if (i == 255) {
    fprintf(stderr, "Line %d: Line exceeded maximum length", lineno);
    
    exit(1);
  }

  transition = parsetransition(buf);

  if (transition != DISPATCH_STATE) {
    for (i = 0; i < 8; i++) {
            for (j = 0; j < 4; j++) {
                    rom[state + (i << OP_OFFSET) + (j << CC_OFFSET)] =
                            (transition & 0x0000ffff) + signals;

                    rom[state + (i << OP_OFFSET) + (j << CC_OFFSET) + Z_SET] =
                            ((transition & 0xffff0000) >> 16) + signals;
            }
    }
  } else {
    for (i = 0; i < 7; i++) {
            for (j = 0; j < 4; j++) {
                    rom[state + (i << OP_OFFSET) + (j << CC_OFFSET)] =
                            dispatchstate[i] + signals;

                    rom[state + (i << OP_OFFSET) + (j << CC_OFFSET) + Z_SET] =
                            dispatchstate[i] + signals;
            }
    }

    /* SPOP CC=i */
    for (i = 0; i < 4; i++) {
            rom[state + (7 << OP_OFFSET) + (i << CC_OFFSET)] =
                    spopstate[i] + signals;

            rom[state + (7 << OP_OFFSET) + Z_SET + (i << CC_OFFSET)] =
                    spopstate[i] + signals;
    }
  }

  return 1;
}

/* The main program reads in command line parameters, opens the
   source file for reading, invokes the main loop calling subroutines
   as needed and then writes out the completed file */
int main(int argc, char **argv) {
  unsigned int i;
  FILE *fp;

  /* Zero out the ROM initially such that if a state is not specified
     it is easy to notice (even halt states have a nonzero value) */
  for (i = 0; i < (1 << 12); i++)
    rom[i] = 0;

  if (argc != 2) {
    fprintf(stderr,
	    "Usage: fsmrom file.fsm\n\tOutputs a compiled ROM to file.hex\n");

    exit(1);
  }

  fp = fopen(argv[1], "r");

  if (fp == NULL) {
    fprintf(stderr, "File %s not found.\n", argv[1]);

    exit(1);
  }
  
  while (handlestate(fp));

  for (i = 0; i < (1 << 11); i++) {
    /* Hardwire in the zero DUMMY state which is the initial state of the
       state machine to assert no control signals and transition to
       state 1 regardless of other (z and cc) inputs */
    if ((i & 0x003f) == 0x00)
      rom[i] = 0x00000001;

    /* deprecated: HALT is no longer hard-coded. */
    /* Hardwire in the HALT state which is the halting state of the state
       machine to assert no control signals and transition back to itself
       regardless of other (z and cc) inputs */
    /* if ((i & 0x003f) == 0x3f)
       rom[i] = 0x0000003f; */
  }

#ifdef DEBUG_VERBOSE
  for (i = 0; i < (1 << 12); i++)
    printf("0x%04X: %08X\n", i, rom[i]);
#endif

  fclose(fp);

  fp = fopen(strcat(strsep(&argv[1], "."), ".hex"), "w");

  for (i = 0; i < (1 << 12); i++) {
          /* LogicWorks wants a less "raw" format than above */
          fprintf(fp, "%08X ", rom[i]);
  }

  fclose(fp);

  return 0;
}
