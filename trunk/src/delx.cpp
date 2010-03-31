/**@name delx.cpp --- �t�@�C����Windows�̃S�~���֑���.
 * @author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>

#include "mydef.h"

//------------------------------------------------------------------------
// �ėp�֐��Q - inline�֐��������̂ŁA�����R���p�C������include�Ŏ�荞��.
//........................................................................
#include "mylib\errfunc.cpp"
//#include "mylib\strfunc.cpp"

//------------------------------------------------------------------------
// �^�A�萔�A�O���[�o���ϐ��̒�`.
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
/** �S�~���փt�@�C���𑗂�.
 * @param fname �t�@�C�����B���C���h�J�[�h�B
 */
int recycle_bin(const char* fname)
{
	char fullname[MY_MAX_PATH+2];
	char* last;

	// SHFileOperation�� fullpath�łȂ��Ǝ󂯕t���Ȃ�
	// �t�@�C���������� "\0\0"�łȂ��Ƃ����Ȃ�
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
/** ���C�� */
int main(int argc, char** argv)
{
	//--- �R�}���h���C����̃I�v�V��������͂���.
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

	//--- �����ɗ^����ꂽ�t�@�C�������ԂɃS�~���֑���.
	if (argc < 2) {
		// �����Ȃ�. usage��\�����ďI������.
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
	<br>�{�\�t�g�E�F�A�͖��ۏ؂������Œ񋟂��܂��B���p�A�Ĕz�z�A���ς͎��R�ł��B

@section intro �͂��߂�
	delx�́A�t�@�C����Windows�̃S�~���֑���R���\�[���A�v���P�[�V�����ł��B
	�R�}���h���C���ォ��S�~���փt�@�C���𑗂肽���ꍇ�ɕ֗��ł��B

@section func ����
	- ���C���h�J�[�h�Ńt�@�C�����w��ł��܂��B

@section env �����
	Windows2000�ȍ~�𓮍�ΏۂƂ��Ă��܂��B
	WindowsXP �ɂē���m�F�ς݁B

@section install �C���X�g�[�����@
	�z�z�t�@�C�� delx.exe ���APATH���ʂ����t�H���_�ɃR�s�[���Ă��������B
	�A�C���C���X�g�[������ɂ́A���̃R�s�[�����t�@�C�����폜���Ă��������B

@section delx-usage �g����
	@@@todo

@section delx-example �o�͗�
	@@@todo

@section links �����N
	- http://code.google.com/p/win32cmdx/ - delx�J���T�C�g

@section download �_�E�����[�h
	- http://code.google.com/p/win32cmdx/downloads/list

@section changelog ��������
	- version-1.0 [Feb xx, 2010] ���J����
	- version-0.1 [Apr 18, 2001] original
*/

// del9.cpp - end.
