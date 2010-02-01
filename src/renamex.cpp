/**@file xrename.cpp -- rename file with pattern.
 * $Id: xrename.cpp,v 1.3 2003/03/19 05:46:44 hkuno Exp $
 * @author Hiroshi Kuno <hkuno@micorhouse.co.jp>
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <mbstring.h>
#include <time.h>
#include <io.h>
#include <process.h>

#include <map>
#include <vector>
#include <string>
using namespace std;

//------------------------------------------------------------------------
// 型、定数、グローバル変数の定義
//........................................................................
// typedef and constants
/** 無符号文字型の別名 */
typedef unsigned char uchar;

/** 無符号long型の別名 */
typedef unsigned long ulong;

//........................................................................
// global variables

/** -c: case sensitive scan */
bool gCaseSensitive = false;

/** -d: sub directory recursive scan */
bool gRecursive = false;

/** -n: test only */
bool gTestOnly = false;

/** -i: ignore error */
bool gIgnoreError = false;

//........................................................................
// messages
/** short help-message */
const char* gUsage  = "usage :xrename [-h?cdni] FROM TO FILES\n";

/** detail help-message for options and version */
const char* gUsage2 =
	"  $Revision: 1.3 $\n"
	"  -h -?  this help\n"
	"  -c     case sensitive scan\n"
	"  -d     sub directory recursive scan\n"
	"  -n     test only, don't rename FILES\n"
	"  -i     ignore error. non-stop mode\n"
	"  FROM   replace from pattern\n"
	"  TO     replace to pattern\n"
	"  FILES  file match pattern(default is '*')\n"
	;

//------------------------------------------------------------------------
///@name 汎用エラー処理関数
//@{
/** usageとエラーメッセージを表示後に、exitする */
void error_abort(const char* msg = NULL)
{
	fputs(gUsage, stderr);
	if (msg)
		fputs(msg, stderr);
	exit(EXIT_FAILURE);
}

void error_abort(const char* prompt, const char* arg)
{
	fputs(gUsage, stderr);
	fprintf(stderr, "%s: %s\n", prompt, arg);
	exit(EXIT_FAILURE);
}

void errorf_abort(const char* fmt, ...)
{
	fputs(gUsage, stderr);
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

/** エラーメッセージと、Win32の詳細エラー情報を表示する */
void print_win32error(const char* msg)
{
	DWORD win32error = ::GetLastError();
	char buf[1000];
	::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, win32error, 0, buf, sizeof(buf), NULL);
	fprintf(stderr, "%s: Win32Error(%d) %s", msg, win32error, buf);
}
//@}

//------------------------------------------------------------------------
///@name 汎用文字列関数群
//@{
/** s1とs2は等しいか? */
inline bool strequ(const char* s1, const char* s2)
{
	return strcmp(s1, s2) == 0;
}

/** 大文字小文字を無視すれば, s1とs2は等しいか? */
inline bool striequ(const char* s1, const char* s2)
{
	return _mbsicmp((const uchar*)s1, (const uchar*)s2) == 0;
}

/** 大文字小文字を無視すれば, s1とs2は等しいか? */
inline bool strniequ(const char* s1, const char* s2, size_t n)
{
	return _mbsnicmp((const uchar*)s1, (const uchar*)s2, n) == 0;
}

/** 大文字小文字を無視すれば, s1の中に部分文字列s2があるか? */
inline char* stristr(const char* s1, const char* s2)
{
	int up = toupper((uchar) s2[0]);
	int lo = tolower((uchar) s2[0]);
	int len = strlen(s2);
	while (*s1) {
		char* upper = strchr(s1, up);
		char* lower = strchr(s1, lo);
		s1 = (!lower || (upper && upper < lower)) ? upper : lower;
		if (!s1)
			return NULL;
		if (strniequ(s1, s2, len))
			return (char*)s1;
		++s1;
	}
	return NULL;
}

//@}

//------------------------------------------------------------------------
///@name ファイル名操作関数群
//@{
/** ワイルドカード記号を持っているか? */
inline bool has_wildcard(const char* pathname)
{
	return strpbrk(pathname, "*?") != NULL;
}

/** パス名を、フォルダ名とファイル名に分離する.
 * @param pathname	解析するパス名.
 * @param folder	分離したフォルダ名の格納先(不要ならNULL可). e.g. "a:\dir\dir\"
 * @param name		分離したファイル名の格納先(不要ならNULL可). e.g. "sample.cpp"
 */
void separate_pathname(const char* pathname, char* folder, char* name)
{
	if (strlen(pathname) >= _MAX_PATH)
		error_abort("too long pathname", pathname);
	char drv[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char base[_MAX_FNAME];
	char ext[_MAX_EXT];
	_splitpath(pathname, drv, dir, base, ext);
	if (folder != NULL)
		_makepath(folder, drv, dir, NULL, NULL);
	if (name != NULL)
		_makepath(name, NULL, NULL, base, ext);
}

/** フォルダ名とファイル名をパス名に結合する.
 * @param pathname	結合したパス名の格納先.
 * @param folder	フォルダ名
 * @param name		ファイル名
 */
void make_pathname(char* pathname, const char* folder, const char* name)
{
	size_t folder_len = strlen(folder);
	if (folder_len >= _MAX_DRIVE + _MAX_DIR)
		error_abort("too long folder name", folder);
	if (strlen(name) >= _MAX_FNAME)
		error_abort("too long file name", name);

	if (folder_len == 2 && folder[1] == ':')
		_makepath(pathname, folder, NULL, name, NULL);
	else
		_makepath(pathname, NULL, folder, name, NULL);
}
//@}

//------------------------------------------------------------------------
/** ファイル検索クラス.
 * _findfirst/next処理のラッパークラスです.
 */
class FindFile : public _finddata_t {
	long mHandle;
public:
	//....................................................................
	/** コンストラクタ. */
	FindFile() : mHandle(-1) {}

	/** デストラクタ. 検索途中ならClose()を行う. */
	~FindFile() {
		if (mHandle != -1)
			Close();
	}

	//....................................................................
	//@{
	/** 検索開始. */
	void Open(const char* dir, const char* wild);
	void Open(const char* pathname);
	//@}

	//@{
	/** 次検索. 終端に達したら自動的にClose()を行う. */
	void Next() {
		if (_findnext(mHandle, this) != 0)
			Close();
	}
	void operator++() {
		Next();
	}
	//@}

	//@{
	/// 検索終了.
	void Close() {
		_findclose(mHandle); mHandle = -1;
	}
	//@}

	//....................................................................
	/** 検索中? */
	bool Opened() const {
		return mHandle != -1;
	}
	/** 検索中? */
	operator const void*() const {
		return Opened() ? this : NULL;
	}
	/** 検索終了? */
	bool operator!() const {
		return !Opened();
	}

	//....................................................................
	/** 検索ファイル名取得. */
	const char* Name() const {
		return name;
	}
	/** フォルダか? */
	bool IsFolder() const {
		return (attrib & _A_SUBDIR) != 0;
	}
	/** 相対フォルダ("." or "..")か? */
	bool IsDotFolder() const;
};

//........................................................................
void FindFile::Open(const char* dir, const char* wild)
{
	char path[_MAX_PATH + _MAX_FNAME + 10]; // パス区切り記号の追加に備えて +10の余裕をとる.
	make_pathname(path, dir, wild);
	Open(path);
}

//........................................................................
void FindFile::Open(const char* pathname)
{
	mHandle = _findfirst(pathname, this);
}

//........................................................................
bool FindFile::IsDotFolder() const
{
	// "." or ".."?
	return IsFolder() && name[0] == '.'
		&& (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}

//------------------------------------------------------------------------
/** リネームを実行する */
void Rename(const char* from, const char* to, const char* dir, const char* wild)
{
	char oldpath[_MAX_PATH + _MAX_FNAME + 10];
	char newpath[_MAX_PATH + _MAX_FNAME + 10];
	char newname[_MAX_FNAME];
	size_t from_len = strlen(from);
	size_t to_len = strlen(to);

	if (strlen(dir) >= _MAX_PATH)
		error_abort("too long folder name", dir);
	if (to_len >= _MAX_FNAME)
		error_abort("too long TO pattern", to);

	FindFile find;
	for (find.Open(dir, wild); find; find.Next()) {
		// フォルダはリネーム対象外.
		if (find.IsFolder())
			continue;

		// パターンに一致しなければ対象外.
		char* match = gCaseSensitive ? strstr(find.name, from) : stristr(find.name, from);
		if (!match)
			continue;

		// パターン置き換えしたファイル名を生成する.
		size_t pre_len = match - find.name;
		strncpy(newname, find.name, pre_len);
		strcpy(newname + pre_len, to);
		strcpy(newname + pre_len + to_len, match + from_len);

		make_pathname(oldpath, dir, find.name);
		make_pathname(newpath, dir, newname);

		// 状況を表示する.
		printf("%s%s => %s\n", dir, find.name, newname);
		if (gTestOnly)
			continue;

		// リネームを実行する.
		if (rename(oldpath, newpath) != 0) {
			print_win32error("rename");
			if (!gIgnoreError)
				error_abort();
		}
	}//.endfor find

	if (gRecursive) {
		// 各サブフォルダに対して再帰する.
		find.Close();
		for (find.Open(dir, "*"); find; find.Next()) {
			if (find.IsFolder() && !find.IsDotFolder()) {
				char subdir[_MAX_PATH + _MAX_FNAME + 10];
				make_pathname(subdir, dir, find.name);
				strcat(subdir, "\\");
				Rename(from, to, subdir, wild);
			}
		}//.endfor find dir
	}
}

//------------------------------------------------------------------------
/** メイン関数 */
int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	//--- コマンドライン上のオプションを解析する.
	while (argc > 1 && argv[1][0]=='-') {
		char* sw = &argv[1][1];
		if (strcmp(sw, "help") == 0)
			goto show_help;
		else {
			do {
				switch (*sw) {
				case 'h': case '?':
show_help:			error_abort(gUsage2);
					break;
				case 'c':
					gCaseSensitive = true;
					break;
				case 'd':
					gRecursive = true;
					break;
				case 'n':
					gTestOnly = true;
					break;
				case 'i':
					gIgnoreError = true;
					break;
				default:
					error_abort("unknown option.\n");
					break;
				}
			} while (*++sw);
		}
//next_arg:
		++argv;
		--argc;
	}
	if (argc < 3) {
		error_abort("please specify FROM TO FILES\n");
	}

	//--- コマンドライン上の FROM TO を取り出す.
	const char* from = const_cast<const char*>(argv[1]);
	const char* to   = const_cast<const char*>(argv[2]);

	//--- リネームを実行する.
	if (argc == 3) {
		Rename(from, to, "", "*"); // default files = "*"
	}
	else {
		for (int i = 3; i < argc; ++i) {
			char dir[_MAX_PATH];
			char name[_MAX_FNAME];
			separate_pathname(argv[i], dir, name);
			Rename(from, to, dir, name);
		}//.endfor
	}
	return EXIT_SUCCESS;
}

// decomment.cpp - end.
