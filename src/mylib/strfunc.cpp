/**@file strfunc.cpp --- C string functions.
 * @author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>
 */
#include <string.h>
#include <mbstring.h>

//------------------------------------------------------------------------
/** s1‚Æs2‚Í“™‚µ‚¢‚©? */
inline bool strequ(const char* s1, const char* s2)
{
	return strcmp(s1, s2) == 0;
}

/** ‘å•¶š¬•¶š‚ğ–³‹‚·‚ê‚Î, s1‚Æs2‚Í“™‚µ‚¢‚©? */
inline bool striequ(const char* s1, const char* s2)
{
	return _mbsicmp((const uchar*)s1, (const uchar*)s2) == 0;
}

/** ‘å•¶š¬•¶š‚ğ–³‹‚·‚ê‚Î, s1‚Æs2‚Í“™‚µ‚¢‚©? */
inline bool strniequ(const char* s1, const char* s2, size_t n)
{
	return _mbsnicmp((const uchar*)s1, (const uchar*)s2, n) == 0;
}

/** ‘å•¶š¬•¶š‚ğ–³‹‚·‚ê‚Î, s1‚Ì’†‚É•”•ª•¶š—ñs2‚ª‚ ‚é‚©? */
inline char* stristr(const char* s1, const char* s2)
{
	int up = toupper((uchar) s2[0]);
	int lo = tolower((uchar) s2[0]);
	int len = strlen(s2);
	while (*s1) {
		const char* upper = strchr(s1, up);
		const char* lower = strchr(s1, lo);
		s1 = (!lower || (upper && upper < lower)) ? upper : lower;
		if (!s1)
			return NULL;
		if (strniequ(s1, s2, len))
			return (char*)s1;
		++s1;
	}
	return NULL;
}

//------------------------------------------------------------------------
/** s1‚Ís2‚æ‚è¬‚³‚¢‚©? */
inline bool strless(const char* s1, const char* s2)
{
	return _mbscmp((const uchar*)s1, (const uchar*)s2) < 0;
}

/** ‘å•¶š¬•¶š‚ğ–³‹‚·‚ê‚Î, s1‚Ís2‚æ‚è¬‚³‚¢‚©? */
inline bool striless(const char* s1, const char* s2)
{
	return _mbsicmp((const uchar*)s1, (const uchar*)s2) < 0;
}

class StrILess {
public:
	bool operator()(const char* s1, const char* s2) const {
		return striless(s1, s2);
	}
};

//------------------------------------------------------------------------
/** ˆóü‰Â”\•¶š‚ğ•Ô‚·. ˆóü•s‰Â”\•¶š‚É‘Î‚µ‚Ä‚Í'.'‚ğ•Ô‚· */
inline int ascii(int c)
{
	return (!iscntrl(c) && isprint(c)) ? c : '.';
}

// strfunc.cpp - end.
