/**@file zipdump.cpp --- ZIPファイルの構造ダンプを行う.
 * @author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <ctype.h>
//using namespace std;

//------------------------------------------------------------------------
// 型、定数、グローバル変数の定義
//........................................................................
// typedef and constants
typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;

#define MY_MAX_PATH	(_MAX_PATH * 2)	// Unicodeで_MAX_PATH文字なので、MBCSではその倍が必要.

//........................................................................
// global variables

/** -f: full dump */
bool gIsFullDump = false;

/** -q: quiet mode */
bool gQuiet = false;

/** -s: output to stdout */
bool gIsStdout = false;

/** -r: recursive search */
bool gIsRecursive = false;

/** -d<DIR>: output folder */
const char* gOutDir = NULL;

//........................................................................
// messages
/** short help-message */
const char* gUsage  = "usage :zipdump [-h?fqsr] [-d<DIR>] file1.zip file2.zip ...\n";

/** detail help-message for options and version */
const char* gUsage2 =
	"  version 1.0 (r8)\n"
	"  -h -?      this help\n"
	"  -f         full dump\n"
	"  -q         quiet mode\n"
	"  -s         output to stdout instend of files(*.zipdump.txt)\n"
	"  -r         recursive search under the input-file's folder(wildcard needed)\n"
	"  -d<DIR>    output to DIR\n"
	"  fileN.cpp  input-files. wildcard OK\n";

//------------------------------------------------------------------------
// 汎用関数群
//........................................................................
// エラー処理系.
/** usageとエラーメッセージを表示後に、exitする */
void error_abort(const char* msg)
{
	fputs(gUsage, stderr);
	if (msg)
		fputs(msg, stderr);
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

//........................................................................
// 文字列処理系.
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

//........................................................................
// ファイル処理系.
/** 入力ファイルオープン.
 * オープン失敗時にはリターンしないで、ここで終了する.
 */
FILE* OpenInput(const char* fname)
{
	FILE* fp = fopen(fname, "rb");
	if (fp == NULL) {
		fprintf(stderr, "can't open input file: %s\n", fname);
		exit(EXIT_FAILURE);
	}
	return fp;
}

/** 出力ファイルオープン.
 * 出力ファイル名は、渡されたファイル名の末尾に extname を追加したものとする.
 * オープン失敗時にはリターンしないで、ここで終了する.
 */
FILE* OpenOutput(const char* inputfname, const char* extname)
{
	char fname[MY_MAX_PATH+100];	// 拡張子には100文字もみておけば十分
	if (gOutDir) {
		char base[MY_MAX_PATH];
		char ext[MY_MAX_PATH];
		_splitpath(inputfname, NULL, NULL, base, ext);
		_makepath(fname, NULL, gOutDir, base, ext);
	}
	else {
		strcpy(fname, inputfname);
	}
	strcat(fname, extname);

	FILE* fp = fopen(fname, "w");
	if (fp == NULL) {
		fprintf(stderr, "can't open output file: %s\n", fname);
		exit(EXIT_FAILURE);
	}
	return fp;
}

//........................................................................
// バイナリ入力処理系.
inline bool Read32(FILE* fin, uint32& val)
{
	return fread(&val, sizeof(val), 1, fin) == 1;
}

inline bool Read16(FILE* fin, uint16& val)
{
	return fread(&val, sizeof(val), 1, fin) == 1;
}

//........................................................................
// ダンプ表示処理系.
void Print_u(FILE* fout, const char* prompt, uint16 a)
{
	fprintf(fout, "%32s : %u\n", prompt, a);
}

void Print_u(FILE* fout, const char* prompt, uint32 a)
{
	fprintf(fout, "%32s : %u\n", prompt, a);
}

void Print_x(FILE* fout, const char* prompt, uint16 a)
{
	fprintf(fout, "%32s : 0x%04X\n", prompt, a);
}

void Print_x(FILE* fout, const char* prompt, uint32 a)
{
	fprintf(fout, "%32s : 0x%08X\n", prompt, a);
}

void Print_ux(FILE* fout, const char* prompt, uint16 a)
{
	fprintf(fout, "%32s : %u(0x%04X)\n", prompt, a, a);
}

void Print_ux(FILE* fout, const char* prompt, uint32 a)
{
	fprintf(fout, "%32s : %u(0x%08X)\n", prompt, a, a);
}

void Print_note(FILE* fout, const char* note)
{
	fprintf(fout, "%32s * %s\n", "", note);
}

void Print_section(FILE* fout, const char* section, int n, __int64 offset = -1)
{
	fprintf(fout, "[%s #%d]", section, n);
	if (offset >= 0 && !gQuiet)
		fprintf(fout, " offset : %I64d(0x%016I64X)", offset, offset);
	fputc('\n', fout);
}

//------------------------------------------------------------------------
// 本体関数群
//........................................................................
/** 次のZIP構造ヘッダまで読み飛ばす. */
void SkipToNextPK(FILE* fin, FILE* fout)
{
	uint64 skipsize = 0;
	int c;
	while ((c = getc(fin)) != EOF) {
		++skipsize;
		if (c != 'P') {
			continue;
		}
		if (getc(fin) != 'K') {
			_fseeki64(fin, -1, SEEK_CUR);
			continue;
		}
		// find "PK" marker.
		_fseeki64(fin, -2, SEEK_CUR);
		--skipsize;
		break;
	}
	if (skipsize > 0) {
		fprintf(fout, "skip unknown data %I64u(0x%I64X) bytes\n", skipsize, skipsize);
	}
}

//........................................................................
// ZIPフィールド内容のダンプ.
void Print_date_and_time(FILE* fout, uint16 mod_time, uint16 mod_date)
{
	// use windows API
	FILETIME ft;
	SYSTEMTIME st;
	::DosDateTimeToFileTime(mod_date, mod_time, &ft);
	::FileTimeToSystemTime(&ft, &st);

	char buf[20*6];
	sprintf_s(buf, sizeof(buf), "%u-%02u-%02uT%02d:%02u:%02u",
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond);
	Print_note(fout, buf);
}

void Print_general_purpose_bit_flag(FILE* fout, uint16 flags, uint16 method)
{
	uint16 w;

	if (flags & 0x0001) Print_note(fout, "Bit 0: encrypted");

	switch (method) {
	case 6:
		// (For Method 6 - Imploding)
		if (flags & 0x0002) Print_note(fout, "Bit 1: Method6: 8K sliding dictionary");
		if (flags & 0x0004) Print_note(fout, "Bit 2: Method6: 3 Shannon-Fano trees");
		break;

	case 8:
	case 9:
		// (For Methods 8 and 9 - Deflating)
		w = (flags >> 1) & 3;
		if (w == 1) Print_note(fout, "Bit 1-2: Method8/9: Maximum (-exx/-ex) compression");
		if (w == 2) Print_note(fout, "Bit 1-2: Method8/9: Fast (-ef) compression");
		if (w == 3) Print_note(fout, "Bit 1-2: Method8/9: Super Fast (-es) compression");
		break;

	case 14:
		// (For Method 14 - LZMA)
		if (flags & 0x0002) Print_note(fout, "Bit 1: Method14: end-of-stream marker used to mark the end of the compressed data stream");
		break;
	}//.endswitch
	
	if (flags & 0x0008) Print_note(fout, "Bit 3: crc-32, compressed size and uncompressed size are set to zero");
	if (flags & 0x0010) Print_note(fout, "Bit 4: Reserved for use with method 8, for enhanced deflating");
	if (flags & 0x0020) Print_note(fout, "Bit 5: compressed patched data.  (Note: Requires PKZIP version 2.70 or greater)");
	if (flags & 0x0040) Print_note(fout, "Bit 6: Strong encryption.");
	if (flags & 0x0080) Print_note(fout, "Bit 7: Currently unused.");
	if (flags & 0x0100) Print_note(fout, "Bit 8: Currently unused.");
	if (flags & 0x0200) Print_note(fout, "Bit 9: Currently unused.");
	if (flags & 0x0400) Print_note(fout, "Bit 10: Currently unused.");
	if (flags & 0x0800) Print_note(fout, "Bit 11: Language encoding flag (EFS). the filename and comment fields for this file must be encoded using UTF-8.");
	if (flags & 0x1000) Print_note(fout, "Bit 12: Reserved by PKWARE for enhanced compression.");
	if (flags & 0x2000) Print_note(fout, "Bit 13: Used when encrypting the Central Directory to indicate selected data values in the Local Header are masked to hide their actual values.");
	if (flags & 0x4000) Print_note(fout, "Bit 14: Reserved by PKWARE.");
	if (flags & 0x8000) Print_note(fout, "Bit 15: Reserved by PKWARE.");
}


//........................................................................
void Dump_string(FILE* fin, FILE* fout, size_t length)
{
	int c;
	while (length-- && (c = getc(fin)) != EOF) {
		fputc(c, fout);
	}
	fputc('\n', fout);
}

void Dump_bytes(FILE* fin, FILE* fout, size_t length)
{
	int c;
	char hex_dump[16*3+1];
	char ascii_dump[16+1];
	size_t i = 0, offset = 0;
	while (length-- && (c = getc(fin)) != EOF) {
		++offset;
		sprintf(hex_dump + i*3, "%02X%c", c, i==7 ? '-' : ' ');
		ascii_dump[i] = ascii(c);
		if (++i >= 16) {
			ascii_dump[16] = 0;
			printf("+%p : %-48s:%-16s\n", offset-i, hex_dump, ascii_dump);
			i = 0;
		}
	}//.endwhile
	if (i != 0) {
		ascii_dump[i] = 0;
		printf("+%p : %-48s:%-16s\n", offset-i, hex_dump, ascii_dump);
	}
}

//........................................................................
// ZIPファイルエントリのダンプ.
void Dump_Local_file(FILE* fin, FILE* fout, uint32 signature, int n)
{
/*
  A.  Local file header:

        local file header signature     4 bytes  (0x04034b50)
        version needed to extract       2 bytes
        general purpose bit flag        2 bytes
        compression method              2 bytes
        last mod file time              2 bytes
        last mod file date              2 bytes
        crc-32                          4 bytes
        compressed size                 4 bytes
        uncompressed size               4 bytes
        file name length                2 bytes
        extra field length              2 bytes

        file name (variable size)
        extra field (variable size)
*/
	uint16 w16, flags, method, mod_time, mod_date, file_name_length, extra_field_length;
	uint32 w32, compressed_size;

	Print_section(fout, "Local file header", n, _ftelli64(fin)-4);

	Print_x(fout, "local file header signature", signature);
	if (Read16(fin, w16)) {
		Print_u(fout, "version needed to extract", w16);
	}
	if (Read16(fin, flags) && Read16(fin, method)) {
		Print_x(fout, "general purpose bit flag", flags);
		if(!gQuiet) Print_general_purpose_bit_flag(fout, flags, method);
		Print_x(fout, "compression method", method);
	}
	if (Read16(fin, mod_time) && Read16(fin, mod_date)) {
		Print_x(fout, "last mod file time", mod_time);
		Print_x(fout, "last mod file date", mod_date);
		if(!gQuiet) Print_date_and_time(fout, mod_time, mod_date);
	}
	if (Read32(fin, w32)) {
		Print_x(fout, "crc-32", w32); // ZIP64では、0xFFFFFFFFに固定する.
	}
	if (Read32(fin, compressed_size)) {
		Print_ux(fout, "compressed size", compressed_size); // ZIP64では、0xFFFFFFFFに固定する.
	}
	if (Read32(fin, w32)) {
		Print_ux(fout, "uncompressed size", w32); // ZIP64では、0xFFFFFFFFに固定する.
	}
	if (Read16(fin, file_name_length)) {
		Print_ux(fout, "file name length", file_name_length); // 65,535以上は不可.
	}
	if (Read16(fin, extra_field_length)) {
		Print_ux(fout, "extra field length", extra_field_length); // 65,535以上は不可.
	}

	Print_section(fout, "Local file name", n, _ftelli64(fin));
	Dump_string(fin, fout, file_name_length);

	Print_section(fout, "Local extra field", n, _ftelli64(fin));
	Dump_bytes(fin, fout, extra_field_length);

/*
  B.  File data

      Immediately following the local header for a file
      is the compressed or stored data for the file. 
      The series of [local file header][file data][data
      descriptor] repeats for each file in the .ZIP archive. 
*/
	Print_section(fout, "File data", n, _ftelli64(fin));
	if (gIsFullDump) {
		Dump_bytes(fin, fout, compressed_size);
	}
	else {
		_fseeki64(fin, compressed_size, SEEK_CUR);
		if (!gQuiet) fprintf(fout, "** skip file data(%lu bytes) **; Use -f option to dump the data\n", compressed_size);
	}

/*
  C.  Data descriptor:

        crc-32                          4 bytes
        compressed size                 4 bytes
        uncompressed size               4 bytes

      This descriptor exists only if bit 3 of the general
      purpose bit flag is set (see below).  It is byte aligned
      and immediately follows the last byte of compressed data.
      This descriptor is used only when it was not possible to
      seek in the output .ZIP file, e.g., when the output .ZIP file
      was standard output or a non-seekable device.  For ZIP64(tm) format
      archives, the compressed and uncompressed sizes are 8 bytes each.

      When compressing files, compressed and uncompressed sizes 
      should be stored in ZIP64 format (as 8 byte values) when a 
      files size exceeds 0xFFFFFFFF.   However ZIP64 format may be 
      used regardless of the size of a file.  When extracting, if 
      the zip64 extended information extra field is present for 
      the file the compressed and uncompressed sizes will be 8
      byte values.  
*/
	if (flags & 0x0008) {
		Print_section(fout, "Data descriptor", n, _ftelli64(fin));
		if (Read32(fin, w32)) {
			Print_x(fout, "crc-32", w32); // ZIP64では、0xFFFFFFFFに固定する.
		}
		if (Read32(fin, compressed_size)) {
			Print_ux(fout, "compressed size", compressed_size); // ZIP64では、0xFFFFFFFFに固定する.
		}
		if (Read32(fin, w32)) {
			Print_ux(fout, "uncompressed size", w32); // ZIP64では、0xFFFFFFFFに固定する.
		}
	}
}

/** finからのPKZIPファイル入力に対して、foutにダンプ出力する. */
void ZipDumpFile(const char* fname, FILE* fin, FILE* fout)
{
	uint32 signature = 0;
	int file_count = 0;
	while (Read32(fin, signature)) {
		switch (signature) {
		case 0x04034b50:
			Dump_Local_file(fin, fout, signature, ++file_count);
			break;
		default:
			fprintf(fout, "unknown signature:%08x\n", signature);
			break;
		}//.endswitch signature

		SkipToNextPK(fin, fout);
	}//.endwhile 
	if (ferror(fin)) {
		print_win32error(fname);
	}
}

/** fnameを読み込み、コメントと余分な空白を除去し、fname+".zipdump.txt"に出力する. */
void DumpMain(const char* fname)
{
	FILE* fin = OpenInput(fname);
	FILE* fout;
	if (gIsStdout) {
		fout = stdout;
		printf("<<< %s >>> begin.\n", fname);
	}
	else {
		fout = OpenOutput(fname, ".zipdump.txt");
	}

	ZipDumpFile(fname, fin, fout);

	fclose(fin);
	if (gIsStdout) {
		printf("<<< %s >>> end.\n\n", fname);
	}
	else {
		fclose(fout);
	}
}

/** ワイルドカード展開と再帰探索付きの DumpMain */
void DumpWildMain(const char* fname)
{
	if (strpbrk(fname, "*?") == NULL) {
		//----- ワイルドカードを含まないパス名の処理
		DumpMain(fname);
	}
	else {
		//----- ワイルドカードを含むパス名の処理
		char path[MY_MAX_PATH + 1000];
		char drv[_MAX_DRIVE];
		char dir[MY_MAX_PATH + 1000];
		char base[MY_MAX_PATH];
		char ext[MY_MAX_PATH];
		_splitpath(fname, drv, dir, base, ext);

		_finddata_t find;
		long h = _findfirst(fname, &find);
		if (h != -1) {
			do {
				if (find.attrib & _A_SUBDIR)
					continue;
				_makepath(path, drv, dir, find.name, NULL);
				// fprintf(stderr, "zipdump: %s\n", find.name);
				DumpMain(path);
			} while (_findnext(h, &find) == 0);
			_findclose(h);
		}
		if (!gIsRecursive)
			return;

		// サブフォルダを検索し、それぞれに対して再帰する
		_makepath(path, drv, dir, "*.*", NULL);
		h = _findfirst(path, &find);
		if (h != -1) {
			do {
				if (!(find.attrib & _A_SUBDIR))
					continue;
				if (strequ(find.name, ".") || strequ(find.name, ".."))
					continue;
				_makepath(path, drv, dir, find.name, NULL);
				strcat(path, "\\");
				strcat(path, base);
				strcat(path, ext);
				// fprintf(stderr, "zipdump recursive: %s\n", path);
				DumpWildMain(path); // 再帰呼び出し.
			} while (_findnext(h, &find) == 0);
			_findclose(h);
		}
	}
}

//------------------------------------------------------------------------
/** メイン関数 */
int main(int argc, char* argv[])
{
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
				case 'f':
					gIsFullDump = true;
					break;
				case 'q':
					gQuiet = true;
					break;
				case 's':
					gIsStdout = true;
					break;
				case 'r':
					gIsRecursive = true;
					break;
				case 'd':
					gOutDir = sw+1;
					goto next_arg;
				default:
					error_abort("unknown option.\n");
					break;
				}
			} while (*++sw);
		}
next_arg:
		++argv;
		--argc;
	}
	if (argc == 1) {
		error_abort("please specify input file.\n");
	}

	//--- コマンドライン上の各入力ファイルを処理する.
	for (int i = 1; i < argc; i++)
		DumpWildMain(argv[i]);

	return EXIT_SUCCESS;
}
//------------------------------------------------------------------------
/**@mainpage dump zip file structure

@version 1.0 (r8)

@author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>

@par License:
	New BSD License
	Copyright &copy; 2010 by Hiroshi Kuno
	<br>本ソフトウェアは無保証かつ無償で提供します。利用、再配布、改変は自由です。

<hr>
@section intro はじめに
	zipdumpは、ZIPファイルの構造をダンプ出力するコンソールアプリケーションです。
	壊れたZIPファイルの解析に便利です。

@section func 特徴
	- PKZIP version 6.3.2[September 28, 2007] のファイルフォーマット仕様に基づき、構造単位でのダンプを行います。
	- 対象ZIPファイル名に ".zipdump.txt" を付加した名前のテキストファイルを生成し、ダンプ結果を出力します。

@section env 動作環境
	Windows2000以降。
	WindowsXP/Vista/Windows7 にて動作確認済み。

@section install インストール方法
	配布ファイル zipdump.exe を、PATHが通ったフォルダにコピーしてください。
	アインインストールするには、そのコピーしたファイルを削除してください。

@section usage 使い方
	@verbinclude usage.tmp

@section example 出力例
	@verbinclude example.tmp

@section pending 欠けている機能
	- なし。

@section links リンク
	- http://code.google.com/p/win32cmdx/ - zipdump開発サイト

@section download ダウンロード
	- http://code.google.com/p/win32cmdx/downloads/list - 最新版 version 1.0 (r8) [Jan 15, 2010]

@section changelog 改訂履歴
	@subsection Rel100 version-1.0 [Jan 15, 2010] 公開初版
*/

// zipdump.cpp - end.
