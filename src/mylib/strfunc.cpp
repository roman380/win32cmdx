/**@file strfunc.cpp --- C string functions.
 * @author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>
 */
#include <string.h>

//------------------------------------------------------------------------
/** s1‚Æs2‚Í“™‚µ‚¢‚©? */
inline bool strequ(const char* s1, const char* s2)
{
	return strcmp(s1, s2) == 0;
}

/** ˆóü‰Â”\•¶š‚ğ•Ô‚·. ˆóü•s‰Â”\•¶š‚É‘Î‚µ‚Ä‚Í'.'‚ğ•Ô‚· */
inline int ascii(int c)
{
	return (!iscntrl(c) && isprint(c)) ? c : '.';
}

// strfunc.cpp - end.
