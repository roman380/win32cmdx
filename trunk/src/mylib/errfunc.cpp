/**@file errfunc.cpp --- error functions.
 * @author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>
 */
#include <stdio.h>
#include <stdlib.h>

//------------------------------------------------------------------------
extern const char* gUsage;

//------------------------------------------------------------------------
/** usageとエラーメッセージを表示後に、exitする */
void error_abort(const char* msg)
{
	fputs(gUsage, stderr);
	if (msg)
		fputs(msg, stderr);
	exit(EXIT_FAILURE);
}

//------------------------------------------------------------------------
// WindowsAPI part.
/** @note _WIN32
 *	Predefined Macros <http://msdn.microsoft.com/en-us/library/b0084kay.aspx>
 *	 - _WIN32	Defined for applications for Win32 and Win64. Always defined.
 *	 - _WIN64	Defined for applications for Win64.
 *	 - _Wp64	Defined when specifying /Wp64.
 */
#ifdef _WIN32
#include <windows.h>

/** エラーメッセージと、Win32の詳細エラー情報を表示する */
void print_win32error(const char* msg)
{
	DWORD win32error = ::GetLastError();
	char buf[1000];
	::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, win32error, 0, buf, sizeof(buf), NULL);
	fprintf(stderr, "%s: Win32Error(%d) %s", msg, win32error, buf);
}

#endif //_WIN32
// errfunc.cpp - end.
