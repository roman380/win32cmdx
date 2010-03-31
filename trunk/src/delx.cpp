/**@name delx.cpp --- ファイルをWindowsのゴミ箱へ送る.
 * @author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>

#include "mydef.h"

//------------------------------------------------------------------------
// 汎用関数群 - inline関数が多いので、分割コンパイルせずincludeで取り込む.
//........................................................................
#include "mylib\errfunc.cpp"
//#include "mylib\strfunc.cpp"

//------------------------------------------------------------------------
// 型、定数、グローバル変数の定義.
//........................................................................
//!@name messages
//@{
/** short help-message */
const char* gUsage  = "usage :delx [-h?] FILE1 FILE2...\n";

/** detail help-message for options and version */
const char* gUsage2 =
	"  version 1.0 (r56)\n"
	"  -h -?  this help\n"
	"  FLIE#  sending to recycler. wildcard OK\n"
	;
//@}

//------------------------------------------------------------------------
/** ゴミ箱へファイルを送る.
 * @param fname ファイル名。ワイルドカード可。
 */
int recycle_bin(const char* fname)
{
	char fullname[MY_MAX_PATH+2];
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

//------------------------------------------------------------------------
/** メイン */
int main(int argc, char** argv)
{
	//--- コマンドライン上のオプションを解析する.
	while (argc > 1 && argv[1][0]=='-') {
		char* sw = &argv[1][1];
		if (strcmp(sw, "-help") == 0)
			goto show_help;
		else {
			do {
				switch (*sw) {
				case 'h': case '?':
show_help:			error_abort(gUsage2);
					break;
				default:
					errorf_abort("-%s: unknown option.\n", sw);
					break;
				}
			} while (*++sw);
		}
//next_arg:
		++argv;
		--argc;
	}

	//--- 引数に与えられたファイルを順番にゴミ箱へ送る.
	if (argc < 2) {
		// 引数なし. usageを表示して終了する.
		goto show_help;
	}
	for (int i = 1; i < argc; ++i)
		recycle_bin(argv[i]);
//	Sleep(100);
	return EXIT_SUCCESS;
}
//------------------------------------------------------------------------
/**@page delx-manual delx.exe - send files to recycler.

@version 1.0 (r56)

@author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>

@par License:
	New BSD License
	<br>Copyright &copy; 2000, 2010 by Hiroshi Kuno
	<br>本ソフトウェアは無保証かつ無償で提供します。利用、再配布、改変は自由です。

@section intro はじめに
	delxは、ファイルをWindowsのゴミ箱へ送るコンソールアプリケーションです。
	コマンドライン上からゴミ箱へファイルを送りたい場合に便利です。

@section func 特徴
	- ワイルドカードでファイルを指定できます。

@section env 動作環境
	Windows2000以降を動作対象としています。
	WindowsXP にて動作確認済み。

@section install インストール方法
	配布ファイル delx.exe を、PATHが通ったフォルダにコピーしてください。
	アインインストールするには、そのコピーしたファイルを削除してください。

@section delx-usage 使い方
	@@@todo

@section delx-example 出力例
	@@@todo

@section links リンク
	- http://code.google.com/p/win32cmdx/ - delx開発サイト

@section download ダウンロード
	- http://code.google.com/p/win32cmdx/downloads/list

@section changelog 改訂履歴
	- version-1.0 [Feb xx, 2010] 公開初版
	- version-0.1 [Apr 18, 2001] original
*/

// del9.cpp - end.
