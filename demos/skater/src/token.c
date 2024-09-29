#include <allegro5/allegro.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

/* declarations of these variables - see token.h for comments on their meaning */
int Error, Lines;
char ErrorText[1024];
struct Tok Token;
A5O_FILE *input = NULL;

static int my_ungetc_c = -1;

/*

	OVERARCHING NOTES ON THE LEVEL PARSER
	-------------------------------------

	We're using something a lot like a recursive descent parser here except that
	it never actually needs to recurse. It is typified by knowing a bunch of
	tokens (which are not necessarily individual characters but may be lumps of
	characters that make sense together - e.g. 'a number' might be a recognised
	token and 2345 is a number) and two functions:

	GetToken - reads a new token from the input.

	ExpectToken - calls GetToken, checks if the token is the one expected and
	if not flags up an error.
	
	A bunch of other functions call these two to read the file input, according
	to what it was decided it should look like. Suppose we want a C-style list
	of numbers that looks like this:

	{ 1, 2, 3, ... }

	Then we might write the function:

		ExpectToken( '{' );
		while(1)
		{
			ExpectToken( number );
			GetToken();
			switch(token)
			{
				default: flag an error; return;
				case '}': return;
				case ',': break;
			}
		}

*/

/*

	Two macros -

		whitespace(v) evaluates to non-zero if 'v' is a character considered to
		be whitespace (i.e. a space, newline, carriage return or tab)

		breaker(v) evaluates to non-zero if 'v' is a character that indicates
		one of the longer tokens (e.g. a number) is over

*/
#define whitespace(v) ((v == ' ') || (v == '\r') || (v == '\n') || (v == '\t'))
#define breaker(v)	(whitespace(v) || (v == '{') || (v == '}') || (v == ','))


/*

	my_fgetc is a direct replacement for fgetc that takes the same parameters
	and returns the same result but counts lines while it goes.

	There is a slight complication here because of the different line endings
	used by different operating systems. The code will accept a newline or a
	carriage return or both in either order. But if a series of alternating
	new lines and carriage returns is found then they are bundled together
	in pairs for line counting.

	E.g.

		\r					- 1 line ending
		\n					- 1 line ending
		\n\r				- 1 line ending
		\r\n\r\n			- 2 line endings
		\r\r				- 2 line endings

*/
static int my_fgetc(A5O_FILE * f)
{
   static int LastChar = '\0';
   int r;
   char TestChar;

   if (my_ungetc_c != -1) {
      r = my_ungetc_c;
      my_ungetc_c = -1;
   }
   else
      r = al_fgetc(f);

   if (r == '\n' || r == '\r') {
      TestChar = (r == '\n') ? '\r' : '\n';

      if (LastChar != TestChar) {
         Lines++;
         LastChar = r;
      } else
         LastChar = '\0';
   } else
      LastChar = r;

   return r;
}

/*
   Hackish way to ungetc a single character.
*/
static void my_ungetc(int c)
{
   my_ungetc_c = c;
}

/*

	GetTokenInner is the guts of GetToken - it reads characters from the input
	file and tokenises them

*/

static void GetTokenInner(void)
{
   char *Ptr = Token.Text;

   /* filter leading whitespace */
   do {
      *Ptr = my_fgetc(input);
   } while (whitespace(*Ptr));

   /* check whether the token can be recognised from the first character alone.
      This is possible if the token is any of the single character tokens (i.e.
      { } and ,) a comment or a string (i.e. begins with a quote) */
   if (*Ptr == '{') {
      Token.Type = TK_OPENBRACE;
      return;
   }

   if (*Ptr == '}') {
      Token.Type = TK_CLOSEBRACE;
      return;
   }

   if (*Ptr == ',') {
      Token.Type = TK_COMMA;
      return;
   }

   if (*Ptr == '\"') {
      Token.Type = TK_STRING;
      /* if this is a string then read until EOF or the next quote. In
         ensuring that we don't overrun the available string storage in the
         Tok struct an extra potential error condition is invoked */
      do {
         *Ptr++ = my_fgetc(input);
      } while (Ptr[-1] != '\"' && !al_feof(input)
               && (Ptr - Token.Text) < 256);
      Ptr[-1] = '\0';
      if (al_feof(input) || (strlen(Token.Text) == 255))
         Error = 1;
      return;
   }

   if (*Ptr == '#') {
      Token.Type = TK_COMMENT;
      /* comments run to end of line, so find the next \r or \n */
      do {
         *Ptr++ = my_fgetc(input);
      } while (Ptr[-1] != '\r' && Ptr[-1] != '\n');
      Ptr[-1] = '\0';
      return;
   }

   Ptr++;
   *Ptr = '\0';

   /* if we're here, the token was not recognisable from the first character
      alone, meaning it is either a number or ill formed */
   while (1) {
      char newc = my_fgetc(input);  /* read new character */

      /* check if this is a terminator or we have hit end of file as in either
         circumstance we should check if what we have makes a valid number */
      if (breaker(newc) || al_feof(input)) {
         /* check first if we have a valid integer quantity. If so fill
            IQuantity with that and cast to double for FQuantity */
         char *eptr;

         my_ungetc(newc);
         Token.IQuantity = strtol(Token.Text, &eptr, 0);
         if (!*eptr) {
            Token.Type = TK_NUMBER;
            Token.FQuantity = (float)Token.IQuantity;
            return;
         }

         /* if not, check if we have a valid floating point quantity. If
            so, fill FQuantity with that and cast to int for IQuantity */
         Token.FQuantity = (float)strtod(Token.Text, &eptr);
         if (!*eptr) {
            Token.Type = TK_NUMBER;
            Token.IQuantity = (int)Token.FQuantity;
            return;
         }

         /* if what we have doesn't make integer or floating point sense
            then this section of the file appears not to be a valid token */
         Token.Type = TK_UNKNOWN;
         return;
      }

      *Ptr++ = newc;
      *Ptr = '\0';
   }
}

/*

	GetToken is a wrapper for GetTokenInner that discards comments

*/
void GetToken(void)
{
   while (1) {
      GetTokenInner();
      if (Token.Type != TK_COMMENT)
         return;
   }
}

/*

	ExpectToken calls GetToken and then compares to the specified type, setting
	an error if the found token does not match the expected type

*/
void ExpectToken(enum TokenTypes Type)
{
   GetToken();
   if (Token.Type != Type)
      Error = 1;
}
