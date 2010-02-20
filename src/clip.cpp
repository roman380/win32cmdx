/**@file clip.cpp --- クリップボード転送フィルタ.
 * @author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>
 */
#include <windows.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "mydef.h"
using namespace std;

//------------------------------------------------------------------------
// 汎用関数群 - inline関数が多いので、分割コンパイルせずincludeで取り込む.
//........................................................................
#include "mylib\errfunc.cpp"
//#include "mylib\strfunc.cpp"

//........................................................................
// global variables

/** -p/paste: paste mode */
bool gPasteMode = false;

/** -s: shrink white spaces */
bool gShrinkSpaces = false;

//........................................................................
// messages
/** short help-message */
const char* gUsage  = "usage :clip [-h?cps] [FILE1 [FILE2 ...]]\n";

/** detail help-message for options and version */
const char* gUsage2 =
	"  version 1.5 (r22)\n"
	"  -h -?     this help\n"
	"  -c -copy  copy from STDIN or FILES to CLIPBOARD (default)\n"
	"  -p -paste paste from CLIPBOARD to STDOUT\n"
	"  -s        shrink white spaces\n"
	;

//------------------------------------------------------------------------
/** クリップボード操作クラス */
class Clipboard {
	bool mOpened;
public:
	//....................................................................
	/** 生成 */
	Clipboard(HWND h = NULL)
		: mOpened(::OpenClipboard(h) != 0) {}

	/** 破棄 */
	~Clipboard() {
		Close();
	}
	
	//....................................................................
	/** 操作可能か */
	bool IsOpened() {
		return mOpened;
	}

	/** 書き込み準備 */
	bool SetEmpty() {
		return ::EmptyClipboard() != 0;
	}

	/** データ書き込み.
	 * システムクリップボードにデータを送り込む。
	 * @param format クリップボード形式
	 * @param data データバッファハンドル.
	 * GlobalAlloc(GHND) で確保し、Unlock状態で渡すこと。
	 * バッファの所有権はシステム側に移るので、本メソッドに渡したバッファは
	 * これ以降操作してはいけない。
	 */
	bool SetData(UINT format, HANDLE data) {
		return ::SetClipboardData(format, data) != NULL;
	}

	/** データ取得
	 * @param format クリップボード形式
	 */
	HGLOBAL GetData(UINT format) {
		return ::GetClipboardData(format);
	}

	/** 操作完了 */
	void Close() {
		if (mOpened)
			::CloseClipboard();
		mOpened = false;
	}

	//....................................................................
	/** テキスト書き込み */
	bool SetText(const std::string& s);

	/** テキスト取得 */
	bool GetText(std::string& s);

};//.endclass Clipboard


bool Clipboard::SetText(const std::string& s)
{
	size_t alloc_size = s.length() + 2;
	HGLOBAL h = GlobalAlloc(GHND, alloc_size);
	if (!h)
		throw runtime_error("no memory");

	LPSTR p = static_cast<LPSTR>(GlobalLock(h));
	s._Copy_s(p, alloc_size, s.length()); // GHNDで確保したのでsの末尾は元から 0 である.
	GlobalUnlock(h);
	return SetData(CF_TEXT, h);
}

bool Clipboard::GetText(std::string& s)
{
	HGLOBAL h = GetData(CF_TEXT);
	if (!h)
		return false;	// テキストデータなし.

	LPSTR p = static_cast<LPSTR>(GlobalLock(h));
	s.assign(p);
	GlobalUnlock(h);
	return true;
}

//------------------------------------------------------------------------
/** 改行コード変換 */
string NLtoCRLF(const string& s, bool lastCRLF = true)
{
	string w;
	w.reserve(s.length());
	char last = 0;
	for (string::const_iterator i = s.begin(); i != s.end(); ++i) {
		char c = *i;
		if (last != '\r' && c == '\n')
			w += '\r';
		w += (last = c);
	}
	if (lastCRLF && last != '\n')
		w += "\r\n";
	return w;
}

/** 改行コード変換 */
string CRLFtoNL(const string& s, bool lastLF = true)
{
	string w;
	w.reserve(s.length());
	char last = 0;
	for (string::const_iterator i = s.begin(); i != s.end(); ++i) {
		char c = *i;
		if (last == '\r' && c == '\n')
			w.erase(w.end() - 1);
		w += (last = c);
	}
	if (lastLF && last != '\n')
		w += "\n";
	return w;
}

/** 複数の空白文字を一個のスペースに変換する. 先頭と末尾の空白文字は削除する. */
string ShrinkSpaces(const string& s)
{
	string w;
	w.reserve(s.length());
	int spaces = 0;
	for (string::const_iterator i = s.begin(); i != s.end(); ++i) {
		char c = *i;
		if (isspace((uchar)c))
			spaces++;
		else {
			if (spaces != 0 && w.length() != 0)
				w += ' ';
			spaces = 0;
			w += c;
		}
	}
	return w;
}

//------------------------------------------------------------------------
/** 転送 */
char CopyText(istream& in, ostream& out)
{
	char c = 0;
	while (in.get(c))
		out.put(c);
	return c;
}

/** 転送 */
char CopyText(const string& s, ostream& out)
{
	char c = 0;
	for (string::const_iterator i = s.begin(); i != s.end(); ++i)
		out.put(c = *i);
	return c;
}

/** ファイル転送 */
void CopyTextFile(const char* fname, ostream& out)
{
	ifstream f(fname);
	if (!f)
		throw runtime_error("can't open: " + string(fname));

	out << "--- begin " << fname << " ---" << endl;

	char c = CopyText(f, out);
	if (c != '\n')
		out << endl;

	out << "--- end " << fname << " ---" << endl;
}

/** 複数ファイル転送 */
void CopyTextFiles(int argc, char** argv, ostream& out)
{
	for (int i = 1; i < argc; ++i) {
		if (i != 1)
			out << endl;
		CopyTextFile(argv[i], out);
	}
}

//------------------------------------------------------------------------
/** メイン.
	コマンドラインを解析し、
	その指定に応じてクリップボードのデータの操作を行う。

	使い方：<DL>
	<DT>C&gt;dir | clip
		<DD>dirの出力(つまり標準入力)をクリップボードにCOPYする

	<DT>C&gt;clip file1.txt file2.txt
		<DD>file1.txt, file2.txtの内容をクリップボードにCOPYする

	<DT>C&gt;clip -paste
		<DD>クリップボードの内容を表示する(標準出力に送る)

	<DT>C&gt;clip -paste &gt;out.txt
		<DD>クリップボードの内容を out.txt に書き出す
	</DL>
 */
int main(int argc, char** argv)
{
	setlocale(LC_ALL, "");

	//--- コマンドライン上のオプションを解析する.
	while (argc > 1 && argv[1][0]=='-') {
		char* sw = &argv[1][1];
		if (strcmp(sw, "help") == 0)
			goto show_help;
		else if (strcmp(sw, "copy") == 0)
			gPasteMode = false;
		else if (strcmp(sw, "paste") == 0)
			gPasteMode = true;
		else {
			do {
				switch (*sw) {
				case 'h': case '?':
show_help:			error_abort(gUsage2);
					break;
				case 'c':
					gPasteMode = false;
					break;
				case 'p':
					gPasteMode = true;
					break;
				case 's':
					gShrinkSpaces = true;
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

	try {
		Clipboard clip;
		if (!clip.IsOpened())
			throw runtime_error("can't open clipboard");

		if (gPasteMode) {
			//----- paste mode
			string s;
			clip.GetText(s);

			s = CRLFtoNL(s);
			if (gShrinkSpaces)
				s = ShrinkSpaces(s);

			cout << s;
		}
		else {
			//----- copy mode
			stringstream ss;
			string s;
			if (argc == 1)
				CopyText(cin, ss);
			else
				CopyTextFiles(argc, argv, ss);

			s = NLtoCRLF(ss.str());
			if (gShrinkSpaces)
				s = ShrinkSpaces(ss.str());

			clip.SetEmpty();
			clip.SetText(s);
		}
		clip.Close();
	}
	catch (runtime_error& e) {
		print_win32error(e.what());
		error_abort();
	}
	catch (exception& e) {
		error_abort(e.what());
	}
	return EXIT_SUCCESS;
}
//------------------------------------------------------------------------
/**@page clip-manual clip.exe - clipboard pipe

@version 1.5 (r22)

@author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>

@par License:
	New BSD License
	<br>Copyright &copy; 2001,2003,2010 by Hiroshi Kuno
	<br>本ソフトウェアは無保証かつ無償で提供します。利用、再配布、改変は自由です。

<hr>
@section intro はじめに
	clipは、クリップボードの内容をコマンドパイプラインに繋げるコマンドです。

@section clip-func 機能
	- 指定ファイルまたは標準入力を読み出し、クリップボードへ格納します。
	- クリップボードの内容を読み出し、標準出力へ出力します。
	- オプション動作として、空白文字を圧縮できます。

@section env 動作環境
	Windows2000以降を動作対象としています。
	WindowsXP にて動作確認済み。

@section install インストール方法
	配布ファイル clip.exe を、PATHが通ったフォルダにコピーしてください。
	アインインストールするには、そのコピーしたファイルを削除してください。

@section usage 使い方
	@verbinclude clip.usage

@section clip-example 使用例
	@verbatim
C>dir | clip
	dirの出力(つまり標準入力)をクリップボードにCOPYする

C>clip file1.txt file2.txt
	file1.txt, file2.txtの内容をクリップボードにCOPYする

C>clip -paste
	クリップボードの内容を表示する(標準出力に送る)

C>clip -paste >out.txt
	クリップボードの内容を out.txt に書き出す
	@endverbatim

@section todo 改善予定
	- 画像ファイルを扱えるようにする.

@section links リンク
	- http://code.google.com/p/win32cmdx/ - clip開発サイト

@section download ダウンロード
	- http://code.google.com/p/win32cmdx/downloads/list - 最新版 version 1.5 (r22) [Jan 17, 2010]

@section changelog 改訂履歴
	@subsection Rel100 version-1.5 [Jan 17, 2010] 公開初版
*/

// clip.cpp - end.
