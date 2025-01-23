LC2200-16 Tools Documentation

=> NOTE: This assembler is intended to help you complete phase 2 of
project 1.

=> Introduction

The LC2200-16 is defined in the Project 1 description along with all its
specific implementation details in the provided datapath.  The assembler in
this package is provided to assist in development and testing of your
LC2200-16 implementation.

=> Example Files

For your initial testing it is suggested that you write shorter (perhaps 1
line) programs to test specific aspects of both your design and FSM.

=> Loading Data into GT Logisim PROMs and RAMs

Once you have used the included tools to generate the code for your
three ROMS and/or test programs in the .hex format, you will need to load them into the correct
places in your GT Logisim datapath. To load the file into either part,
first, open up one of the generated HEX files (example.hex for the standard microcontroller;
beqrom.txt, mainrom.txt, and seqrom.txt for the bonus microcontroller; or a converted assembly program), 
hit "ctrl+a" to highlight all the text and "ctrl+v" to copy
it to the clipboard. Then, right click on the correct part in GT Logisim,
select "Edit Contents..." from the menu, select ONLY the first byte in GT
Logisim's HEX editor, and paste the copied HEX code into the ROM/RAM.
Click "Save" to save your changes. The data is now loaded into the PROM/RAM module.


=> 16-bit Assembler

The LC2200-16 assembler is a single-pass, table look-back assembler for the
instruction set specified in the Project 1 description.  For additional
documentation please see the detailed comments in the assembler source code.

To use the assembler, enter the following at the console:
assemble16-win32 <source>.s

This will produce an output file called <source>.hex which may then be loaded
as a hex input file into GT Logisim.  See above for a note on the formatting of
this file.

Note that for Project 1 there is no requirement to write any actual code for
your LC2200-16 implementation, but it will be extremely helpful in debugging
your implementation as well as instructive in understanding how the computer
operates.

The assembler is whitespace neutral and case insensitive for mnemonics,
so generally you may format you file in whatever way you like.  A good general
guideline on how to format your file, however, is to place labels on the
left margin, mnemonics aligned on the first tabstop, argument lists aligned on
the second tabstop, and then have your argument list however you like.
Comments way occur anywhere by using the ';' character, but continue to the
end of the line.  There are no inline /* */ C-style comments.  In general
the LC2200-16 assembler follows most of the best practices for RISC-based
architectures and is specifically modeled after MIPS.

---
25 Jan. 2005: Initial revision, Kyle Goodwin
30 Jan. 2005: Revised for release, Kyle Goodwin
28 May  2005: Minor Touch-up, Matt Balaun
12 Sep. 2006: Revised for 16 bit, Kane Bonnette
19 May  2008: Cleaned up SPOP/HALT confusion, James Robinson
28 Aug. 2008: Added Windows binaries information and did some cleanup, Matt Bigelow
15 Sep. 2010: Re-written for the MICOCompiler three-ROM state machine. Assembler left untouched, Charlie Shilling
20 Jan. 2011: Split into several files: README-assembler.txt, README-standard.txt, and README-bonus.txt, Charlie Shilling