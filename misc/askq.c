/*
 *  ASKQ - asks a yes/no question, returning an exit status to the caller.
 *  This is used by the msvcmake installation batch file.
 */


#include <conio.h>
#include <ctype.h>



int main(int argc, char *argv[])
{
   int i;

   cputs("\r\n");

   for (i=1; i<argc; i++) {
      if (i > 1)
	 putch(' ');

      cputs(argv[i]);
   }

   cputs("? [y/n] ");

   for (;;) {
      i = getch();

      if ((tolower(i) == 'y') || (tolower(i) == 'n')) {
	 putch(i);
	 cputs("\r\n\n");
	 return (tolower(i) == 'y') ? 0 : 1;
      }
      else
	 putch(7);
   }
}
