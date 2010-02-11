/* Empty file */

/*
 * MSVC will not generate an empty .lib file
 */
#ifdef _MSC_VER
__declspec(dllexport) void _al_make_a_lib()
{
	exit(1);
}
#endif
