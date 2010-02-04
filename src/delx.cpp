/**@name del9.cpp
 * Windows95/98/NT4/2000ゴミ箱転送付きの delコマンド
 * @author Hiroshi Kuno
 *      <P> Copyleft 2000 hkuno.kuno@nifty.ne.jp
 * @version 0.1
 * $Id: del9.cpp,v 1.1 2001/04/18 06:51:53 hkuno Exp $
 * @make bcc32 -WC -x- -RT- -X -O1 del9.cpp noeh32.lib
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shellapi.h>

/** ゴミ箱へファイルを送る
 * @param fname ファイル名。ワイルドカード可。
 */
int recycle_bin(char* fname)
{
	char fullname[1000];
	char* last;

	// SHFileOperationは fullpathでないと受け付けない
	// ファイル名末尾は "\0\0"でないといけない
	GetFullPathName(fname, sizeof(fullname)-1, fullname, &last);
	fullname[strlen(fullname)+1] = '\0';

	SHFILEOPSTRUCT op;
	ZeroMemory(&op, sizeof(op));
	op.hwnd = GetDesktopWindow();
	op.wFunc = FO_DELETE;
	op.pFrom = fullname;
//	op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
	op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION;
	return SHFileOperation(&op);
}

/** メイン */
int main(int argc, char** argv)
{
	if (argc < 2) {
		fputs(
			"del9 - ごみ箱送りのdelコマンド\n"
			"usage: del9 *.bak\n",
			stderr);
		return EXIT_FAILURE;
	}
	for (int i = 1; i < argc; ++i)
		recycle_bin(argv[i]);
//	Sleep(100);
	return EXIT_SUCCESS;
}
// del9.cpp - end.
