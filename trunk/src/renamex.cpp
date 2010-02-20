/**@file renamex.cpp --- rename file with pattern.
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
//!@name option settings
//@{
/** -c: case sensitive scan */
bool gCaseSensitive = false;

/** -r: sub directory recursive scan */
bool gRecursive = false;

/** -n: test only */
bool gTestOnly = false;

/** -i: ignore error */
bool gIgnoreError = false;
//@}

//........................................................................
//!@name messages
//@{
/** short help-message */
const char* gUsage  = "usage :renamex [-h?cdni] FROM TO FILES\n";

/** detail help-message for options and version */
const char* gUsage2 =
	"  version 1.3 (r48)\n"
	"  -h -?  this help\n"
	"  -c     case sensitive scan\n"
	"  -d     sub directory recursive scan\n"
	"  -n     test only, don't rename FILES\n"
	"  -i     ignore error. non-stop mode\n"
	"  FROM   replace from pattern\n"
	"  TO     replace to pattern\n"
	"  FILES  file match pattern(default is '*')\n"
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
/** ���l�[�������s���� */
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
		// �t�H���_�̓��l�[���ΏۊO.
		if (find.IsFolder())
			continue;

		// �p�^�[���Ɉ�v���Ȃ���ΑΏۊO.
		char* match = gCaseSensitive ? strstr(find.name, from) : stristr(find.name, from);
		if (!match)
			continue;

		// �p�^�[���u�����������t�@�C�����𐶐�����.
		size_t pre_len = match - find.name;
		strncpy(newname, find.name, pre_len);
		strcpy(newname + pre_len, to);
		strcpy(newname + pre_len + to_len, match + from_len);

		make_pathname(oldpath, dir, find.name);
		make_pathname(newpath, dir, newname);

		// �󋵂�\������.
		printf("%s%s => %s\n", dir, find.name, newname);
		if (gTestOnly)
			continue;

		// ���l�[�������s����.
		if (rename(oldpath, newpath) != 0) {
			print_win32error("rename");
			if (!gIgnoreError)
				error_abort();
		}
	}//.endfor find

	if (gRecursive) {
		// �e�T�u�t�H���_�ɑ΂��čċA����.
		find.Close();
		for (find.Open(dir, "*"); find; find.Next()) {
			if (find.IsFolder() && !find.IsDotFolder()) {
				if (find.IsHidden() || find.IsSystem()) continue;
				if (strequ(find.name, "CVS")) continue;
				char subdir[_MAX_PATH + _MAX_FNAME + 10];
				make_pathname(subdir, dir, find.name);
				strcat(subdir, "\\");
				Rename(from, to, subdir, wild);
			}
		}//.endfor find dir
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
				case 'c':
					gCaseSensitive = true;
					break;
				case 'r':
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

	//--- �R�}���h���C����� FROM TO �����o��.
	const char* from = const_cast<const char*>(argv[1]);
	const char* to   = const_cast<const char*>(argv[2]);

	//--- ���l�[�������s����.
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
//------------------------------------------------------------------------
/**@page renamex-manual renamex.exe - rename file with pattern

@version 1.3 (r48)

@author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>

@par License:
	New BSD License
	<br>Copyright &copy; 2003,2010 by Hiroshi Kuno
	<br>�{�\�t�g�E�F�A�͖��ۏ؂������Œ񋟂��܂��B���p�A�Ĕz�z�A���ς͎��R�ł��B

<hr>
@section intro �͂��߂�
	renamex�́A�t�@�C�����̈ꕔ�����ꊇ�p�^�[���u������R�}���h�ł��B

@section renamex-func ����
	- ���C���h�J�[�h�Ń��l�[���Ώۂ��w��ł��܂��B
	- �t�H���_�ċA�����Ńt�H���_�ȉ����ꊇ�������邱���\�ł��B

@section env �����
	Windows2000�ȍ~�𓮍�ΏۂƂ��Ă��܂��B
	WindowsXP �ɂē���m�F�ς݁B

@section install �C���X�g�[�����@
	�z�z�t�@�C�� renamex.exe ���APATH���ʂ����t�H���_�ɃR�s�[���Ă��������B
	�A�C���C���X�g�[������ɂ́A���̃R�s�[�����t�@�C�����폜���Ă��������B

@section usage �g����
	@verbinclude renamex.usage

@section renamex-example �g�p��
	@verbatim
	@@@Todo Here!!
	@endverbatim

@section todo ���P�\��
	- �Ȃ��B

@section links �����N
	- http://code.google.com/p/win32cmdx/ - renamex�J���T�C�g

@section download �_�E�����[�h
	- http://code.google.com/p/win32cmdx/downloads/list

@section changelog ��������
	- version-1.3 [Feb xx, 2010] ���J����
*/

// renamex.cpp - end.
