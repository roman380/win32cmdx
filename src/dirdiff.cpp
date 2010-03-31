/**@file dirdiff.cpp -- compare and diff folder.
 * @author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>
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

#include "mydef.h"
using namespace std;

//------------------------------------------------------------------------
// �ėp�֐��Q - inline�֐��������̂ŁA�����R���p�C������include�Ŏ�荞��.
//........................................................................
#include "mylib\errfunc.cpp"
#include "mylib\strfunc.cpp"

//------------------------------------------------------------------------
// �^�A�萔�A�O���[�o���ϐ��̒�`
//........................................................................
/** ISO 8601�`���ɐ��`����strftime()�̏��� */
#define ISO8601FMT	"%Y-%m-%dT%H:%M:%S"

//........................................................................
//!@name option settings
//@{
/** -s: ignore same file date */
bool gIgnoreSameFileDate = false;

/** -r: ignore right only file */
bool gIgnoreRightOnlyFile = false;

/** -l: ignore left only file */
bool gIgnoreLeftOnlyFile = false;

/** -d: diff for file */
bool gDiff = false;

/** -t,-T: time format */
const char* gTmFmt = ISO8601FMT;
//@}

//........................................................................
//!@name messages
//@{
/** short help-message */
const char* gUsage  = "usage :dirdiff [-h?srlutTd] DIR1 [DIR2] [WILD]\n";

/** detail help-message for options and version */
const char* gUsage2 =
	"  version 1.6 (r57)$\n"
	"  -h -?  this help\n"
	"  -s     ignore same file date\n"
	"  -r     ignore right only file\n"
	"  -l     ignore left  only file\n"
	"  -u     ignore unique file(same as -r -l)\n"
	"  -t     use locale time format\n"
	"  -T     use ISO 8601 time format(default)\n"
	"  -d     diff for file\n"
	"  DIR1   compare folder\n"
	"  DIR2   compare folder(default is current-folder)\n"
	"  WILD   file match pattern(default is '*')\n"
	;
//@}

//------------------------------------------------------------------------
// �ėp�֐��Q
//........................................................................
///@name �t�@�C��������֐��Q
//@{
/** ���C���h�J�[�h�L���������Ă��邩? */
inline bool has_wildcard(const char* pathname)
{
	return strpbrk(pathname, "*?") != NULL;
}

/** �p�X�����A�t�H���_���ƃt�@�C�����ɕ�������.
 * @param pathname	��͂���p�X��.
 * @param folder	���������t�H���_���̊i�[��(�s�v�Ȃ�NULL��). e.g. "a:\dir\dir\"
 * @param name		���������t�@�C�����̊i�[��(�s�v�Ȃ�NULL��). e.g. "sample.cpp"
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

/** �t�H���_���ƃt�@�C�������p�X���Ɍ�������.
 * @param pathname	���������p�X���̊i�[��.
 * @param folder	�t�H���_��
 * @param name		�t�@�C����
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
/** �t�@�C�������N���X.
 * _findfirst/next�����̃��b�p�[�N���X�ł�.
 */
class FindFile : public _finddata_t {
	long mHandle;
public:
	//....................................................................
	/** �R���X�g���N�^. */
	FindFile() : mHandle(-1) {}

	/** �f�X�g���N�^. �����r���Ȃ�Close()���s��. */
	~FindFile() {
		if (mHandle != -1)
			Close();
	}

	//....................................................................
	//@{
	/** �����J�n. */
	void Open(const char* dir, const char* wild);
	void Open(const char* pathname);
	//@}

	//@{
	/** ������. �I�[�ɒB�����玩���I��Close()���s��. */
	void Next() {
		if (_findnext(mHandle, this) != 0)
			Close();
	}
	void operator++() {
		Next();
	}
	//@}

	//@{
	/// �����I��.
	void Close() {
		_findclose(mHandle); mHandle = -1;
	}
	//@}

	//....................................................................
	/** ������? */
	bool Opened() const {
		return mHandle != -1;
	}
	/** ������? */
	operator const void*() const {
		return Opened() ? this : NULL;
	}
	/** �����I��? */
	bool operator!() const {
		return !Opened();
	}

	//....................................................................
	/** �����t�@�C�����擾. */
	const char* Name() const {
		return name;
	}
	/** �t�H���_��? */
	bool IsFolder() const {
		return (attrib & _A_SUBDIR) != 0;
	}
	/** �B��������? */
	bool IsHidden() const {
		return (attrib & _A_HIDDEN) != 0;
	}
	/** �V�X�e��������? */
	bool IsSystem() const {
		return (attrib & _A_SYSTEM) != 0;
	}
	/** ���΃t�H���_("." or "..")��? */
	bool IsDotFolder() const;
};

//........................................................................
void FindFile::Open(const char* dir, const char* wild)
{
	char path[_MAX_PATH + _MAX_FNAME + 10]; // �p�X��؂�L���̒ǉ��ɔ����� +10�̗]�T���Ƃ�.
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
/** �t�@�C���ꗗ���쐬���� */
void MakeFileList(vector<_finddata_t>& vec, const char* dir, const char* wild)
{
	FindFile find;
	for (find.Open(dir, wild); find; find.Next()) {
		// �t�H���_�͎��W�Ώۂ��珜�O����.
		if (find.IsFolder())
			continue;

		// vector �̐L��������������.
		if (vec.capacity() == vec.size())
			vec.reserve(vec.size() + 1024);

		// vector�Ɍ����t�@�C����o�^����.
		vec.push_back(find);
	}
}

/** ���݂���t�H���_�ł��邱�Ƃ�ۏ؂���. ������肪����ΏI������. */
void ValidateFolder(const char* dir)
{
	DWORD attr = ::GetFileAttributes(dir);
	if (attr == -1) {
		print_win32error(dir);
		error_abort();
	}
	if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		error_abort("not a folder", dir);
	}
}

//------------------------------------------------------------------------
/** ���E�̃t�@�C����� */
class Entry {
public:
	Entry()
		: Left(NULL), Right(NULL) { }
	const _finddata_t* Left;
	const _finddata_t* Right;

	int print(FILE* fp) const;
};

/** �\��.
 * @param fp	�o�͐�
 * @retval -1	ignored
 * @retval 0	left only / right only
 * @retval =	same
 * @retval <	right is newer
 * @retval >	left is newer
 */
int Entry::print(FILE* fp) const
{
	time_t l = Left  ? Left->time_write  : 0;
	time_t r = Right ? Right->time_write : 0;
	char lbuf[100];
	char rbuf[100];
	int mark = 0;
	if (Left && Right) {
		if (l == r) {
			if (gIgnoreSameFileDate) return -1;
			mark = '=';
		}
		else if (l < r)
			mark = '<';
		else
			mark = '>';
		strftime(lbuf, sizeof(lbuf), gTmFmt, localtime(&l));
		strftime(rbuf, sizeof(rbuf), gTmFmt, localtime(&r));
		fprintf(fp, "[ %s ] %c [ %s ] %s\n", lbuf, mark, rbuf, Left->name);
		return mark;
	}
	else if (Left) {
		if (gIgnoreLeftOnlyFile) return -1;
		size_t n = strftime(lbuf, sizeof(lbuf), gTmFmt, localtime(&l));
		fprintf(fp, "[ %s ]     %*s   %s\n", lbuf, n, "", Left->name);
	}
	else if (Right) {
		if (gIgnoreRightOnlyFile) return -1;
		size_t n = strftime(rbuf, sizeof(rbuf), gTmFmt, localtime(&r));
		fprintf(fp, "  %*s     [ %s ] %s\n", n, "", rbuf, Right->name);
	}
	return mark;
}

//------------------------------------------------------------------------
/** �t�H���_��r�����s���� */
void Compare(const char* dir1, const char* dir2, const char* wild)
{
	// dir1, dir2 ���L���ȃt�H���_�����ׂ�.
	ValidateFolder(dir1);
	ValidateFolder(dir2);

	printf("folder compare [ %s ] <-> [ %s ] with \"%s\"\n", dir1, dir2, wild);

	// dir1, dir2 �̃t�@�C���ꗗ�𓾂�.
	vector<_finddata_t> files1, files2;
	vector<_finddata_t>::const_iterator i;
	MakeFileList(files1, dir1, wild);
	MakeFileList(files2, dir2, wild);

	// �t�@�C�������L�[�Ƃ���}�b�v�ɁA���t�@�C���ꗗ�̗v�f��o�^����.
	map<const char*, Entry, StrILess> list;
	map<const char*, Entry, StrILess>::const_iterator j;
	for (i = files1.begin(); i != files1.end(); ++i)
		list[i->name].Left = &*i;
	for (i = files2.begin(); i != files2.end(); ++i)
		list[i->name].Right = &*i;

	// ���������}�b�v�̓��e��\������.
	for (j = list.begin(); j != list.end(); ++j) {
		int ret = j->second.print(stdout);

		// �t�@�C����diff���Ƃ�A���e���{���ɈقȂ��Ă��邩�m�F����
		if (ret > 0 && gDiff) {
			char file1[_MAX_PATH];
			char file2[_MAX_PATH];
			_makepath(file1, NULL, dir1, j->second.Left->name, NULL);
			_makepath(file2, NULL, dir2, j->second.Right->name, NULL);
			spawnlp(_P_WAIT, "diff", "diff", "-Bwqs", file1, file2, NULL);
		}
	}
}

//------------------------------------------------------------------------
/** ���C���֐� */
int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	//--- �R�}���h���C����̃I�v�V��������͂���.
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
				case 's':
					gIgnoreSameFileDate = true;
					break;
				case 'l':
					gIgnoreLeftOnlyFile = true;
					break;
				case 'r':
					gIgnoreRightOnlyFile = true;
					break;
				case 'u':
					gIgnoreLeftOnlyFile = gIgnoreRightOnlyFile = true;
					break;
				case 'd':
					gDiff = true;
					break;
				case 't':
					gTmFmt = "%c";	// locale time format.
					break;
				case 'T':
					gTmFmt = ISO8601FMT;
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
	if (argc < 2) {
		error_abort("please specify DIR1\n");
	}

	//--- �R�}���h���C����� DIR1 [DIR2] [WILD] �����o��.
	char dir1[_MAX_PATH];
	char dir2[_MAX_PATH];
	char wild[_MAX_PATH];
	strcpy(dir1, argv[1]);
	strcpy(dir2, argc > 2 ? argv[2] : ".");
	strcpy(wild, argc > 3 ? argv[3] : "*");
	if (argc <= 3 && has_wildcard(dir1))
		separate_pathname(dir1, dir1, wild);

	//--- �t�H���_��r�����s����.
	Compare(dir1, dir2, wild);

	return EXIT_SUCCESS;
}
//------------------------------------------------------------------------
/**@page dirdiff-manual dirdiff.exe - show differ of directories

@version 1.6 (r57)

@author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>

@par License:
	New BSD License
	<br>Copyright &copy; 1989, 1990, 2003, 2005, 2010 by Hiroshi Kuno
	<br>�{�\�t�g�E�F�A�͖��ۏ؂������Œ񋟂��܂��B���p�A�Ĕz�z�A���ς͎��R�ł��B

<hr>
@section intro �͂��߂�
	dirdiff�́A�f�B���N�g���Ԃ̃t�@�C�����X�g�̍��ق��r����c�[���ł�.

@section dirdiff-func ����
	- ���C���h�J�[�h�Ń��l�[���Ώۂ��w��ł��܂��B

@section env �����
	Windows2000�ȍ~�𓮍�ΏۂƂ��Ă��܂��B
	WindowsXP �ɂē���m�F�ς݁B

@section install �C���X�g�[�����@
	�z�z�t�@�C�� dirdiff.exe ���APATH���ʂ����t�H���_�ɃR�s�[���Ă��������B
	�A�C���C���X�g�[������ɂ́A���̃R�s�[�����t�@�C�����폜���Ă��������B

@section dirdiff-usage �g����
	@verbinclude dirdiff.usage

@section dirdiff-example �g�p��
	@verbatim
	@@@Todo Here!!
	@endverbatim

@section todo ���P�\��
	- �Ȃ�.

@section links �����N
	- http://code.google.com/p/win32cmdx/ - dirdiff�J���T�C�g

@section download �_�E�����[�h
	- http://code.google.com/p/win32cmdx/downloads/list

@section changelog ��������
	- version-1.6 [Feb xx, 2010] ���J����
	- version-1.5 [Oct 19, 2005] original
*/

// dirdiff.cpp - end.