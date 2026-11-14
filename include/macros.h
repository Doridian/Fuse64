#ifndef MACROS_H
#define MACROS_H

/* Helpers the game's C uses in more than one file. */

/* ovl_17_04 compares with 0.0f, and IDO's code for the two sides of the
 * test is what this spelling gives. */
#define ABS(x) ((x) < 0.0f ? -(x) : (x))

/* The original's debug output, compiled out of the ROM (C89: one
 * parenthesised argument list, `DEBUG_PRINT(("...", x))`). The tests
 * around the calls stay in the code: IDO keeps an emptied if's load and
 * branch in the places it is used. Its messages in src/ are placeholders:
 * only the tests are known. */
#define DEBUG_PRINT(args)

#endif
