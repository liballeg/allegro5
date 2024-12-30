/*
 *  tkeniser.h
 *  Allegro Demo Game
 *
 *  Created by Thomas Harte on 18/07/2005.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __TOKENISER_H
#define __TOKENISER_H

#include <stdio.h>

/*

        Level loading related variables -

        Lines is a count of the number of lines of text that have been parsed from
        the source level file. It is used for error reporting where necessary.

        Error is an error flag. If it is set to non-zero then some error occurred.

        ErrorText is a textual description of the error that has occurred in level
        loading. It is set initially to be an empty string, and if an error is
        flagged but an error string not provided then the user gets an "Unspecified
        error"

*/
extern int Error, Lines;
extern char ErrorText[1024];

/*

        TokenTypes gives a complete list of all the tokens that the parser can
        understand from a file. Most are self explanatory but nevertheless:

                TK_OPENBRACE        - the { character
                TK_CLOSEBRACE        - the } character
                TK_COMMA                - the , character

                TK_STRING                - a section of text enclosed in quotes, e.g. "hello"
                TK_NUMBER                - a number in any C standard format, integer or
                                                floating point (e.g. 97, 012, 0x8 or 23.6f)
                TK_COMMENT                - a comment, which begins with a hash and then runs
                                                to the end of line

                TK_UNKNOWN                - something the parser didn't really understand

*/
enum TokenTypes {
   TK_OPENBRACE, TK_CLOSEBRACE, TK_COMMA,

   TK_STRING, TK_NUMBER, TK_COMMENT,

   TK_UNKNOWN
};

/*

        struct Tok holds a Token. It is straightforward.

        'Type' holds the token type.

        'Text' holds the characters read from the file and taken to be the token
        unless the token is taken to be a string, in which case the surrounding
        quotes are removed

        In the case that a number is found, FQuantity holds a floating point value
        and IQuantity holds an integer value

*/
struct Tok {
   enum TokenTypes Type;

   char Text[256];
   double FQuantity;
   int IQuantity;
};

extern struct Tok Token;
extern ALLEGRO_FILE *input;                /* the file from which level input is read */

extern void GetToken(void);
extern void ExpectToken(enum TokenTypes Type);

#endif
