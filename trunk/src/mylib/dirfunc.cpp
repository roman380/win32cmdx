/**@file dirfunc.cpp --- pathname and directory functions.
 * @author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>
 */
#include <stdio.h>
#include <stdlib.h>
#include <io.h>

//------------------------------------------------------------------------
extern void error_abort(const char* prompt, const char* arg);

//------------------------------------------------------------------------
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

// dirfunc.cpp - end.
