/**@file clip.cpp --- �N���b�v�{�[�h�]���t�B���^.
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
// �ėp�֐��Q - inline�֐��������̂ŁA�����R���p�C������include�Ŏ�荞��.
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
/** �N���b�v�{�[�h����N���X */
class Clipboard {
	bool mOpened;
public:
	//....................................................................
	/** ���� */
	Clipboard(HWND h = NULL)
		: mOpened(::OpenClipboard(h) != 0) {}

	/** �j�� */
	~Clipboard() {
		Close();
	}
	
	//....................................................................
	/** ����\�� */
	bool IsOpened() {
		return mOpened;
	}

	/** �������ݏ��� */
	bool SetEmpty() {
		return ::EmptyClipboard() != 0;
	}

	/** �f�[�^��������.
	 * �V�X�e���N���b�v�{�[�h�Ƀf�[�^�𑗂荞�ށB
	 * @param format �N���b�v�{�[�h�`��
	 * @param data �f�[�^�o�b�t�@�n���h��.
	 * GlobalAlloc(GHND) �Ŋm�ۂ��AUnlock��Ԃœn�����ƁB
	 * �o�b�t�@�̏��L���̓V�X�e�����Ɉڂ�̂ŁA�{���\�b�h�ɓn�����o�b�t�@��
	 * ����ȍ~���삵�Ă͂����Ȃ��B
	 */
	bool SetData(UINT format, HANDLE data) {
		return ::SetClipboardData(format, data) != NULL;
	}

	/** �f�[�^�擾
	 * @param format �N���b�v�{�[�h�`��
	 */
	HGLOBAL GetData(UINT format) {
		return ::GetClipboardData(format);
	}

	/** ���슮�� */
	void Close() {
		if (mOpened)
			::CloseClipboard();
		mOpened = false;
	}

	//....................................................................
	/** �e�L�X�g�������� */
	bool SetText(const std::string& s);

	/** �e�L�X�g�擾 */
	bool GetText(std::string& s);

};//.endclass Clipboard


bool Clipboard::SetText(const std::string& s)
{
	size_t alloc_size = s.length() + 2;
	HGLOBAL h = GlobalAlloc(GHND, alloc_size);
	if (!h)
		throw runtime_error("no memory");

	LPSTR p = static_cast<LPSTR>(GlobalLock(h));
	s._Copy_s(p, alloc_size, s.length()); // GHND�Ŋm�ۂ����̂�s�̖����͌����� 0 �ł���.
	GlobalUnlock(h);
	return SetData(CF_TEXT, h);
}

bool Clipboard::GetText(std::string& s)
{
	HGLOBAL h = GetData(CF_TEXT);
	if (!h)
		return false;	// �e�L�X�g�f�[�^�Ȃ�.

	LPSTR p = static_cast<LPSTR>(GlobalLock(h));
	s.assign(p);
	GlobalUnlock(h);
	return true;
}

//------------------------------------------------------------------------
/** ���s�R�[�h�ϊ� */
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

/** ���s�R�[�h�ϊ� */
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

/** �����̋󔒕�������̃X�y�[�X�ɕϊ�����. �擪�Ɩ����̋󔒕����͍폜����. */
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
/** �]�� */
char CopyText(istream& in, ostream& out)
{
	char c = 0;
	while (in.get(c))
		out.put(c);
	return c;
}

/** �]�� */
char CopyText(const string& s, ostream& out)
{
	char c = 0;
	for (string::const_iterator i = s.begin(); i != s.end(); ++i)
		out.put(c = *i);
	return c;
}

/** �t�@�C���]�� */
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

/** �����t�@�C���]�� */
void CopyTextFiles(int argc, char** argv, ostream& out)
{
	for (int i = 1; i < argc; ++i) {
		if (i != 1)
			out << endl;
		CopyTextFile(argv[i], out);
	}
}

//------------------------------------------------------------------------
/** ���C��.
	�R�}���h���C������͂��A
	���̎w��ɉ����ăN���b�v�{�[�h�̃f�[�^�̑�����s���B

	�g�����F<DL>
	<DT>C&gt;dir | clip
		<DD>dir�̏o��(�܂�W������)���N���b�v�{�[�h��COPY����

	<DT>C&gt;clip file1.txt file2.txt
		<DD>file1.txt, file2.txt�̓��e���N���b�v�{�[�h��COPY����

	<DT>C&gt;clip -paste
		<DD>�N���b�v�{�[�h�̓��e��\������(�W���o�͂ɑ���)

	<DT>C&gt;clip -paste &gt;out.txt
		<DD>�N���b�v�{�[�h�̓��e�� out.txt �ɏ����o��
	</DL>
 */
int main(int argc, char** argv)
{
	setlocale(LC_ALL, "");

	//--- �R�}���h���C����̃I�v�V��������͂���.
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
	<br>�{�\�t�g�E�F�A�͖��ۏ؂������Œ񋟂��܂��B���p�A�Ĕz�z�A���ς͎��R�ł��B

<hr>
@section intro �͂��߂�
	clip�́A�N���b�v�{�[�h�̓��e���R�}���h�p�C�v���C���Ɍq����R�}���h�ł��B

@section clip-func �@�\
	- �w��t�@�C���܂��͕W�����͂�ǂݏo���A�N���b�v�{�[�h�֊i�[���܂��B
	- �N���b�v�{�[�h�̓��e��ǂݏo���A�W���o�͂֏o�͂��܂��B
	- �I�v�V��������Ƃ��āA�󔒕��������k�ł��܂��B

@section env �����
	Windows2000�ȍ~�𓮍�ΏۂƂ��Ă��܂��B
	WindowsXP �ɂē���m�F�ς݁B

@section install �C���X�g�[�����@
	�z�z�t�@�C�� clip.exe ���APATH���ʂ����t�H���_�ɃR�s�[���Ă��������B
	�A�C���C���X�g�[������ɂ́A���̃R�s�[�����t�@�C�����폜���Ă��������B

@section usage �g����
	@verbinclude clip.usage

@section clip-example �g�p��
	@verbatim
C>dir | clip
	dir�̏o��(�܂�W������)���N���b�v�{�[�h��COPY����

C>clip file1.txt file2.txt
	file1.txt, file2.txt�̓��e���N���b�v�{�[�h��COPY����

C>clip -paste
	�N���b�v�{�[�h�̓��e��\������(�W���o�͂ɑ���)

C>clip -paste >out.txt
	�N���b�v�{�[�h�̓��e�� out.txt �ɏ����o��
	@endverbatim

@section todo ���P�\��
	- �摜�t�@�C����������悤�ɂ���.

@section links �����N
	- http://code.google.com/p/win32cmdx/ - clip�J���T�C�g

@section download �_�E�����[�h
	- http://code.google.com/p/win32cmdx/downloads/list - �ŐV�� version 1.5 (r22) [Jan 17, 2010]

@section changelog ��������
	@subsection Rel100 version-1.5 [Jan 17, 2010] ���J����
*/

// clip.cpp - end.
