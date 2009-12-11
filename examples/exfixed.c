/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to use fixed point numbers, which
 *    are signed 32-bit integers storing the integer part in the
 *    upper 16 bits and the decimal part in the 16 lower bits. This
 *    example also uses the unusual approach of communicating with
 *    the user exclusively via the allegro_message() function.
 */


#include <allegro.h>



int main(void)
{
   /* declare three 32 bit (16.16) fixed point variables */
   fixed x, y, z1, z2, z3, z4;

   if (allegro_init() != 0)
      return 1;

   /* convert integers to fixed point like this */
   x = itofix(10);

   /* convert floating point to fixed point like this */
   y = ftofix(3.14);

   /* fixed point variables can be assigned, added, subtracted, negated,
    * and compared just like integers, eg: 
    */
   z1 = x + y;

   /* you can't add integers or floating point to fixed point, though:
    *    z = x + 3;
    * would give the wrong result.
    */

   /* fixed point variables can be multiplied or divided by integers or
    * floating point numbers, eg:
    */
   z2 = y * 2;

   /* you can't multiply or divide two fixed point numbers, though:
    *    z = x * y;
    * would give the wrong result. Use fixmul() and fixdiv() instead, eg:
    */
   z3 = fixmul(x, y);

   /* fixed point trig and square root are also available, eg: */
   z4 = fixsqrt(x);

   allegro_message(
       "%f + %f = %f\n"
       "%f * 2 = %f\n"
       "%f * %f = %f\n"
       "fixsqrt(%f) = %f\n",
       fixtof(x), fixtof(y), fixtof(z1),
       fixtof(y), fixtof(z2),
       fixtof(x), fixtof(y), fixtof(z3),
       fixtof(x), fixtof(z4));

   return 0;
}
END_OF_MAIN()
