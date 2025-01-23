/*
 * CS2200 Summer 2005
 * Project 1, LC2200-16
 * 16-bit Assembler
 *
 * 21 Jan. 2005, Initial Revision, Kyle Goodwin
 * 29 Jan. 2005, Added pseudoinstructions, fixed bugs, Kyle Goodwin
 * 30 Jan. 2005, Fixed a bug with LA, Kyle Goodwin
 * 11 Jul. 2005, Modified for C-simulator-formatted output, Matt Balaun
 * 19 Jul. 2005, Modified for additional error reporting, Matt Balaun
 * 25 Aug. 2005, Modified offset field functionality, Matt Balaun
 * 14 Jan. 2006, Modified to handle white space, comments, Kamaram Munira
 * 20 May  2006, Handles bash-style comments, improved error reports, Kane Bonnette
 * 19 Aug. 2006, Converting to 16 bit datapath, Kane Bonnette
 * 17 Sep. 2010, Updated to work with the new R-type definition (see line 421), Connor Wakamo
 *
 * This is a single pass assembler for the LC2200-16 machine
 * with forward reference resolution using a labels table.
 *
 * Instructions are 1-word long, each word is 2 bytes, byte = 8 bits.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* A table of error messages in printf format */

#define OFF_SIZE_ERR \
 "Line %d: Reference to label '%s' on line %d is too far away for branch\n"

#define ABS_SIZE_ERR \
 "Line %d: Reference to label '%s' on line %d has too large an address\n"

#define IMM_OUT_RANGE \
 "Line %d: Immediate value %d out of range, 5-bits allowed\n"

#define CTRL_CODE_ERR \
 "Line %d: Control code '%d' out of range, must be 0-3, inclusive\n"

#define REG_NAME_ERR \
 "Line %d: Unrecognized register name '%s'\n"

#define NO_MNEMONIC_ERR \
 "Line %d: No mnemonic (instruction) supplied\n"

#define FORMAT_ERR \
 "Line %d: Malformed instruction or incorrect number of arguments\n"

#define LABEL_ERR \
 "Line %d: Label '%s' must be terminated with ':'\n"

#define DATA_RANGE_ERR \
 "Line %d: Data value out of range for the specified data type\n"

#define UNKNOWN_TYPE_ERR \
 "Line %d: Unknown data type '%s'\n"

/* Defines the register names and numbers */
const char *regnames[16] = {
        "$zero",
        "$at",
        "$v0",
        "$a0",
        "$a1",
        "$a2",
        "$t0",
        "$t1",
        "$t2",
        "$s0",
        "$s1",
        "$s2",
        "$k0",
        "$sp",
        "$pr",
        "$ra"
};

/* Defines a structure to serve as the basis for a linked list of labels */
typedef struct label {
        char name[65];
        unsigned short address; 
        unsigned int lineno;    /* For error reporting purposes, the line
                                   that this reference occured on */
        struct label *next;
        struct label *prev;
} LABEL;



/* Defines a structure to serve as the basis for a linked list of
   unresolved references to forward or non-present labels */
typedef struct unresolved {
        char name[65];
        unsigned short partialop;
        unsigned char la;
        unsigned int lineno;     /* For error reporting purposes, the line
                                    that this reference occured on */
        unsigned short address;
        struct unresolved *next;
        struct unresolved *prev;
} UNRESOLVED;

/* Now declare a linked list of labels and another of unresolved references */
LABEL *labels = NULL;
UNRESOLVED *refs = NULL;

/* Because the datapath is 16-bit, we have 65536 possible 16-bit words */
unsigned short memimage[65536];

/* The global address counter is used to determine the address in memory
   of the instruction/data currently being assembled */
unsigned short address = 0;

/* In order to report helpful error messages, a line counter is used */
unsigned int lineno = 0;

/* The LC2200-16 architecture specifies that programs always start at address
   0 and proceed to higher values in memory so we initialize the address to
   be 0, count upwards, and finally output an assembled program which is a
   memory image of size equal to the address of the highest instruction/data
   element and beginning at address 0 */

/* Emits LA instructions when both addresses are known */
void dola(UNRESOLVED *ref, LABEL *label, unsigned char regno) {
        unsigned short source;
        unsigned short target;
        unsigned short instruction;

        /* Determine if it is backwards or forwards */
        if (ref != NULL) {
                /* It was a forwards reference, emit code at ref->address
                   for an LA address */

                source = ref->address;
                target = address;
        } else {
                /* It was a backwards reference, emit code at address for
                   an LA label->address */

                source = address;
                target = label->address;
        }


        /* ADDI to load $pr */
        instruction = 0x5C00 + ((target & 0x0F00) >> 8);
        memimage[source] = instruction;
        source++;

        /* ADDI to load 5 hob of target register */
        instruction = (2 << 13) + (regno << 9) + ((target & 0x00F8) >> 3);

        memimage[source] = instruction;
        source++;

        /* ADDs multiply by 2 three times to shift bits in target register */
        instruction = (regno << 9) + (regno << 5) + (regno << 0);

        memimage[source] = instruction;
        source++;

        memimage[source] = instruction;
        source++;

        memimage[source] = instruction;
        source++;

        /* ADDI to load 3 lob of target register */
        instruction = (2 << 13) + (regno << 9) + (regno << 5) +
                (target & 0x0007);

        memimage[source] = instruction;
        source++;
}

void dolabel(char *label) {
        LABEL *curlabel;
        UNRESOLVED *curref;
        unsigned char offset;
        char *temp;

        while (isspace(*label)) label++;

        temp = strchr(label, ':');

        if (temp == NULL) {
                fprintf(stderr, LABEL_ERR, lineno, label);
                exit(1);
        }



        *temp = '\0';

        /* If there ar any unresolved references currently outstanding
           iterate through them to see if this label resolves any of them */
        if (refs != NULL) {
                for (curref = refs; curref != NULL; curref = curref->next) {
                        /* If the name of the label matches then the label
                           resolves this reference, so complete assembly
                           of the partially assembled instruction and remove
                           the unresolved reference from the linked list */
                        if (!strcmp(label, curref->name)) {
                                /* If the operation is a BEQ instruction then
                                   we must insert a relative address, otherwise
                                   we use an absolute address */
                                if ((curref->partialop & 0xE000) == 0xA000) {
                                        /* Check to see if the offset is
                                           too large to fit in the 5 bits:
                                           this must be a forward (>0) offset
                                           and since the offset is signed it
                                           must be smaller than the largest
                                           possible 4-bit value, 15 */
                                        offset = address -
                                                curref->address - 1;

                                        if (offset > 15) {
                                                fprintf(stderr, OFF_SIZE_ERR,
                                                        curref->lineno,
                                                        curref->name,
                                                        lineno);
                                                exit(1);
                                        }

                                        curref->partialop += offset;
				
                                        memimage[curref->address] = curref->partialop;
                                } else {
                                        /* Check to see if the absolute address
                                           is too large to fit in the immediate
                                           field of 5 (unsigned) bits */
                                        if (curref->la) {
                                                dola(curref, 0, curref->la);
                                                goto tail;
                                        }
                                        if (curref->address > 31) {
                                                fprintf(stderr, ABS_SIZE_ERR,
                                                        curref->lineno,
                                                        curref->name,
                                                        lineno);
                                                exit(1);
                                        }

                                        curref->partialop += address;
                                        memimage[curref->address] = curref->partialop;
                                }

                                /* Remove the reference from the unresolved
                                   linked list since it is now resolved */
                        tail:   if (curref == refs) {
                                        refs = curref->next;
                                        if (refs != NULL)
                                                refs->prev = NULL;

                                        free(curref);
                                } else if (curref->next == NULL) {
                                        curref->prev->next = NULL;

                                        free(curref);
                                } else {
                                        curref->prev->next = curref->next;
                                        curref->next->prev = curref->prev;

                                        free(curref);
                                }
                        }
                }
        }

        /* Insert this label into the labels table for future use */
        if (labels == NULL) {
                labels = (LABEL *) malloc(sizeof(LABEL));
                labels->next = NULL;
                labels->prev = NULL;

                strcpy(labels->name, label);
                labels->address = address;
                labels->lineno = lineno;
        } else {
                curlabel = labels;

                while (curlabel->next != NULL) curlabel = curlabel->next;

                curlabel->next = (LABEL *) malloc(sizeof(LABEL));
                curlabel->next->next = NULL;
                curlabel->next->prev = curlabel;

                strcpy(curlabel->next->name, label);
                curlabel->next->address = address;
                curlabel->next->lineno = lineno;
        }
}

void doref(char *ref, unsigned short partialop, unsigned char la) {
        LABEL *curlabel;
        UNRESOLVED *curref;

        short offset;

        /* If there are any currently defined labels, check to see if this
           reference is to one of them before deciding that it is unresolved */
        if (labels != NULL) {
                curlabel = labels;

                while (curlabel != NULL) {
                        /* If we find the label this references, resolve the
                           reference and insert the completed op in memory */
                        if (!strcmp(curlabel->name, ref)) {
                                /* If the operation is a BEQ instruction then
                                   we must insert a relative address, otherwise
                                   we use an absolute address */
                                if ((partialop & 0xE000) == 0xA000) {
                                        /* Check to see if the offset is
                                           too large to fit in the 5 bits:
                                           this must be a reverse (<0) offset
                                           and since the offset is signed it
                                           must be larger than the smallest
                                           possible 4-bit value, -15 */
                                        offset = curlabel->address
                                                - address - 1;

                                        if (offset < -15) {
                                                fprintf(stderr, OFF_SIZE_ERR,
                                                        lineno,
                                                        curlabel->name,
                                                        curlabel->lineno);
                                                exit(1);
                                        }

                                        partialop += (0x1F & offset);
                                        memimage[address] = partialop;
                                } else {
                                        /* Check to see if the absolute address
                                           is too large to fit in the immediate
                                           field of 5 (unsigned) bits */
                                        if (la) {
                                                dola(0, curlabel, la);
                                                return;
                                        }
                                        if (curlabel->address > 31) {
                                                fprintf(stderr, ABS_SIZE_ERR,
                                                        lineno,
                                                        curlabel->name,
                                                        curlabel->lineno);
                                                exit(1);
                                        }

                                        partialop += curlabel->address;
                                        memimage[address] = partialop;
                                }

                                return;
                        }

                        curlabel = curlabel->next;
                }
        }

        /* At this point all currently known labels have been traversed
           and the reference remains unresolved, so we create a new
           entry in the unresolved reference linked list for it */

        if (refs == NULL) {
                refs = (UNRESOLVED *) malloc(sizeof(UNRESOLVED));

                refs->next = NULL;
                refs->prev = NULL;

                strcpy(refs->name, ref);
                refs->address = address;
                refs->lineno = lineno;
                refs->partialop = partialop;
                refs->la = la;
        } else {
                curref = refs;

                while (curref->next != NULL) curref = curref->next;

                curref->next = (UNRESOLVED *) malloc(sizeof(UNRESOLVED));

                curref->next->prev = curref;
                curref->next->next = NULL;

                strcpy(curref->next->name, ref);
                curref->next->address = address;
                curref->next->lineno = lineno;
                curref->next->partialop = partialop;
                curref->next->la = la;
        }
}

unsigned char parsereg(char *reg) {
        int regno = 0;
        reg = strsep(&reg, " ");
        while ((regno < 16) && (strcasecmp(reg, regnames[regno]))) regno++;

        if (regno >= 16) {
                fprintf(stderr, REG_NAME_ERR, lineno, reg);
                exit(1);
        }

        return regno;
}

unsigned char packneg(int t) {

        return ((~((-t) & 0x1F) + 1) & 0x1F);
}

void doinstruction(char *mnem, char *arg1, char *arg2, char *arg3) {
        unsigned short op = 0;
        int s;
        unsigned char t;
       mnem = strsep(&mnem, " ");
       arg1 = strsep(&arg1, " ");
       arg2 = strsep(&arg2, " ");
       arg3 = strsep(&arg3, " ");
			
			/* Updated lines 431, 436 to work with the "new" R-type instruction:
			 * Bits 15-13: opcode
			 * Bits 12-9: RX
			 * Bits 8-5: RY
			 * Bit 4: unused
			 * Bits 3-0: RZ
			 */

        if (!strcasecmp(mnem, "add")) {
                /* R-type ADD */
                op += parsereg(arg3) << 0;
                op += parsereg(arg2) << 5;
                op += parsereg(arg1) << 9;
        } else if (!strcasecmp(mnem, "nand")) {
                /* R-type NAND */
                op += parsereg(arg3) << 0;
                op += parsereg(arg2) << 5;
                op += parsereg(arg1) << 9;
                op += 1 << 13;
        } else if (!strcasecmp(mnem, "addi")) {
                /* I-type ADDI */
                op += parsereg(arg2) << 5;
                op += parsereg(arg1) << 9;
                op += 2 << 13;

                if (isdigit(*arg3) || (*arg3 == '-') || (*arg3 == '+')) {
                        s = atoi(arg3);

                        if (s < 0) t = packneg(s);
                        else t = s & 0xFF;

                        if ((t & 0xE0) != 0) {
                                fprintf(stderr, IMM_OUT_RANGE, lineno, t);
                                exit(1);
                        }

                        op += t;
                } else {
                        doref(arg3, op, 0);

                        return;
                }
        } else if (!strcasecmp(mnem, "lw")) {
                /* I-type LW */
                op += parsereg(arg3) << 5;
                op += parsereg(arg1) << 9;
                op += 3 << 13;

                if (isdigit(*arg2) || (*arg2 == '-') || (*arg2 == '+')) {
                        s = atoi(arg2);

                        if (s < 0) t = packneg(s);
                        else t = s & 0xFF;

                        if ((t & 0xE0) != 0) {
                                fprintf(stderr, IMM_OUT_RANGE, lineno, t);
                                exit(1);
                        }

                        op += t;
                } else {
                        doref(arg2, op, 0);

                        return;
                }
        } else if (!strcasecmp(mnem, "sw")) {
                /* I-type SW */
                op += parsereg(arg3) << 5;
                op += parsereg(arg1) << 9;
                op += 4 << 13;

                if (isdigit(*arg2) || (*arg2 == '-') || (*arg2 == '+')) {
                        s = atoi(arg2);

                        if (s < 0) t = packneg(s);
                        else t = s & 0xFF;

                        if ((t & 0xE0) != 0) {
                                fprintf(stderr, IMM_OUT_RANGE, lineno, t);
                                exit(1);
                        }

                        op += t;
                } else {
                        doref(arg2, op, 0);

                        return;
                }
        } else if (!strcasecmp(mnem, "beq")) {
                /* I-type BEQ */
                op += parsereg(arg2) << 5;
                op += parsereg(arg1) << 9;
                op += 5 << 13;

                if (isdigit(*arg3) || (*arg3 == '-') || (*arg3 == '+')) {
                        s = atoi(arg3);

                        if (s < 0) t = packneg(s);
                        else t = s & 0xFF;

                        if ((t & 0xE0) != 0) {
                                fprintf(stderr, IMM_OUT_RANGE, lineno, t);
                                exit(1);
                        }

                        op += t;
                } else {
                        doref(arg3, op, 0);

                        return;
                }
        } else if (!strcasecmp(mnem, "jalr")) {
                /* J-type JALR */
                op += parsereg(arg2) << 5;
                op += parsereg(arg1) << 9;
                op += 6 << 13;
        } else if (!strcasecmp(mnem, "spop")) {
                /* S-type SPOP */
                op += 7 << 13;

                t = atoi(arg1);

                if (t > 3) {
                        fprintf(stderr, CTRL_CODE_ERR, lineno, t);
                        exit(1);
                }

                op += t;
        }

        memimage[address] = op;
}

void dodata(char *type, char *data) {
        int tbyte;
        int tdw;

        if ((!strcasecmp(type, ".word")) || (!strcasecmp(type, ".byte"))) {
                /* Data length is one byte, read it in, check range, store */
                tbyte = atoi(data);

                if ((tbyte > 127) || (tbyte < -128)) {
                        fprintf(stderr, DATA_RANGE_ERR, lineno);
                        exit(1);
                }
		
                memimage[address] = tbyte;

                address++;
        } else if (!strcasecmp(type, ".dw")) {
                /* Data length is two bytes, read it in, check range, store */
                tdw = atoi(data);

                if ((tdw > 32767) || (tdw < -32768)) {
                        fprintf(stderr, DATA_RANGE_ERR, lineno);
                        exit(1);
                }
		
                memimage[address] = tdw;

                address++;
        } else {
                fprintf(stderr, UNKNOWN_TYPE_ERR, lineno, type);
                exit(1);
        }
}

/* translate one line of assembly to machine code */
void doline(char *line) {
        char *mnemonic = NULL;
        char *arg1 = NULL;
        char *arg2 = NULL;
        char *arg3 = NULL;
        while (isspace(*line)) line++;

        if (*line == ';') return;
        if (*line == '!')  return;
	if (*line == '#') return;
        if (*line == '\n') return;
        if (*line == '\r') return;
        if (*line == '\0') return;

        mnemonic = strsep(&line, " \t\n\r");
        while (isspace(*line)) line++;

        if (strchr(mnemonic, ':') != NULL) {
                dolabel(mnemonic);
                if (*line == ';') return;
        	if (*line == '!')  return;
		if (*line == '#') return;
                if (*line == '\n') return;
                if (*line == '\r') return;
                if (*line == '\0') return;
                mnemonic = strsep(&line, " \t");
        }


        if ((mnemonic == NULL) || (*mnemonic == '\0')) {
                fprintf(stderr, NO_MNEMONIC_ERR, lineno);
                exit(1);
        }

        if (*mnemonic == '.') {
                while (isspace(*line)) line++;
                arg1 = strsep(&line, " \t\r\n;");
                dodata(mnemonic, arg1);
                return;
        }

        if (!strcasecmp(mnemonic, "jalr")) {
                /* JALR has only two arguments */
                while (isspace(*line)) line++;
                arg1 = strsep(&line, ",");
                while (isspace(*line)) line++;
                arg2 = strsep(&line, " ;\t\n\r");
                arg3 = NULL;
        } else if (!strcasecmp(mnemonic, "spop")) {
                /* SPOP has only one argument */
                while (isspace(*line)) line++;
                arg1 = strsep(&line, " ;\t\n\r");
                arg2 = NULL;
                arg3 = NULL;
        } else if (!strcasecmp(mnemonic, "noop")) {
                /* NOOP is a pseudoinstruction for add $zero, $zero, $zero */
                mnemonic = strdup("add");
                arg1 = strdup("$zero");
                arg2 = strdup("$zero");
                arg3 = strdup("$zero");
        } else if (!strcasecmp(mnemonic, "halt")) {
                /* HALT is a pseudoinstruction for spop 0 */
                mnemonic = strdup("spop");
                arg1 = strdup("0");
                arg2 = NULL;
                arg3 = NULL;
        } else if (!strcasecmp(mnemonic, "la")) {
                /* LA is a complicated pseudoinstruction which emits
                   6 assembly instructions to load both registers */
                while (isspace(*line)) line++;
                arg1 = strsep(&line, ",");
                while (isspace(*line)) line++;
                arg2 = strsep(&line, " ;\t\n\r");
                arg3 = NULL;

                doref(arg2, 0, parsereg(arg1)); /* Special LA doref */

                /* We have to manually increment the address since this
                   will take six instructions and then return to bypass
                   the doinstruction call at the end of the if-else */

                address += 6;
                return;
        } else if ((!strcasecmp(mnemonic, "add")) ||
		   (!strcasecmp(mnemonic, "nand")) ||
	  	   (!strcasecmp(mnemonic, "addi")) ||
		   (!strcasecmp(mnemonic, "lw")) ||
		   (!strcasecmp(mnemonic, "sw")) ||
		   (!strcasecmp(mnemonic, "beq"))) {
                /* All other instructions have 3 arguments */
                while (isspace(*line)) line++;
                arg1 = strsep(&line, ",");
                while (isspace(*line)) line++;
                arg2 = strsep(&line, ",(");
                while (isspace(*line)) line++;
                arg3 = strsep(&line, " );\t\n\r");
        } else if (*line == ':'){
			 fprintf(stderr, LABEL_ERR, lineno, line);
		                exit(1);
	} else {
		fprintf(stderr, FORMAT_ERR, lineno);
                exit(1);
	}
        doinstruction(mnemonic, arg1, arg2, arg3);
        address++;
}

int main(int argc, char **argv) {
        FILE *fp;
        unsigned short i;
        char buf[256];

        if (argc != 2) {
                fprintf(stderr, "Usage: ./assemble file.s\n");
                fprintf(stderr, "\tOutputs the assembled file.hex\n");
                exit(1);
        }
        if (strstr(argv[1], ".s") == NULL){
		fprintf(stderr, "The file passed in does not have the correct extension\n");
		fprintf(stderr, "Usage: ./assemble file.s\n");
		fprintf(stderr, "\tOutputs the assembled file.hex\n");
                exit(1);
	}

        if (NULL == (fp = fopen(argv[1], "r"))) {
                fprintf(stderr, "File %s not found or could not be opened.\n",
			argv[1]);
                exit(1);
        }

	/* translate the lines one by one */
        while (fgets(buf, 256, fp) != NULL) {
                lineno++;
                doline(buf);
        }

        fclose(fp);

	/* unreferenced label */
        if (refs != NULL) {
                fprintf(stderr,
                        "%d: Unresolved reference to undefined label '%s'.\n",
                        refs->lineno, refs->name);
                exit(1);
        }

	/* create and open the output file */
        if (NULL == (fp = fopen(strcat(strsep(&argv[1], "."), ".hex"), "w"))) {
                fprintf(stderr, "Could not open output file for writing.\n");
                exit(1);
        }

	/* write out the machine code. */
        for (i = 0; i < address; i++) {
          fprintf(fp, "%04X ", memimage[i]);
        }

        fclose(fp);

        return 0;
}
