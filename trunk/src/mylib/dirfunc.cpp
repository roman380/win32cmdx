/**@file dirfunc.cpp --- pathname and directory functions.
 * @author Hiroshi Kuno <http://code.google.com/p/win32cmdx/>
 */
#include <stdio.h>
#include <stdlib.h>
#include <io.h>

//------------------------------------------------------------------------
extern void error_abort(const char* prompt, const char* arg);

//------------------------------------------------------------------------
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
	/** 隠し属性か? */
	bool IsHidden() const {
		return (attrib & _A_HIDDEN) != 0;
	}
	/** システム属性か? */
	bool IsSystem() const {
		return (attrib & _A_SYSTEM) != 0;
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

// dirfunc.cpp - end.
