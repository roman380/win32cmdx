/**@file strfunc.cpp --- C string functions.
 * @author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>
 */
#include <string.h>

//------------------------------------------------------------------------
/** s1とs2は等しいか? */
inline bool strequ(const char* s1, const char* s2)
{
	return strcmp(s1, s2) == 0;
}

/** 印刷可能文字を返す. 印刷不可能文字に対しては'.'を返す */
inline int ascii(int c)
{
	return (!iscntrl(c) && isprint(c)) ? c : '.';
}

// strfunc.cpp - end.
