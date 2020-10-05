﻿#include "Shlobj.h"
#include "tchar.h""
#include "fstream"

#include "stdlib.h"
#include "direct.h"
#include "string.h"
#include "string"
#include "io.h"
#include "stdio.h" 
#include "vector"
#include "iostream"
using namespace std;
TCHAR szBuffer[MAX_PATH] = { 0 };

class CBrowseDir
{
protected:
	//存放初始目录的绝对路径，以'\'结尾
	char m_szInitDir[_MAX_PATH];

public:
	//缺省构造器
	CBrowseDir();

	//设置初始目录为dir，如果返回false，表示目录不可用
	bool SetInitDir(const char* dir);

	//开始遍历初始目录及其子目录下由filespec指定类型的文件
	//filespec可以使用通配符 * ?，不能包含路径。
	//如果返回false，表示遍历过程被用户中止
	bool BeginBrowse(const char* filespec);
	vector<string> BeginBrowseFilenames(const char* filespec);

protected:
	//遍历目录dir下由filespec指定的文件
	//对于子目录,采用迭代的方法
	//如果返回false,表示中止遍历文件
	bool BrowseDir(const char* dir, const char* filespec);
	vector<string> GetDirFilenames(const char* dir, const char* filespec);
	//函数BrowseDir每找到一个文件,就调用ProcessFile
	//并把文件名作为参数传递过去
	//如果返回false,表示中止遍历文件
	//用户可以覆写该函数,加入自己的处理代码
	virtual bool ProcessFile(const char* filename);

	//函数BrowseDir每进入一个目录,就调用ProcessDir
	//并把正在处理的目录名及上一级目录名作为参数传递过去
	//如果正在处理的是初始目录,则parentdir=NULL
	//用户可以覆写该函数,加入自己的处理代码
	//比如用户可以在这里统计子目录的个数
	virtual void ProcessDir(const char* currentdir, const char* parentdir);
};

CBrowseDir::CBrowseDir()
{
	//用当前目录初始化m_szInitDir
	getcwd(m_szInitDir, _MAX_PATH);

	//如果目录的最后一个字母不是'\',则在最后加上一个'\'
	int len = strlen(m_szInitDir);
	if (m_szInitDir[len - 1] != '\\')
		strcat(m_szInitDir, "\\");
}

bool CBrowseDir::SetInitDir(const char* dir)
{
	//先把dir转换为绝对路径
	if (_fullpath(m_szInitDir, dir, _MAX_PATH) == NULL)
		return false;

	//判断目录是否存在
	if (_chdir(m_szInitDir) != 0)
		return false;

	//如果目录的最后一个字母不是'\',则在最后加上一个'\'
	int len = strlen(m_szInitDir);
	if (m_szInitDir[len - 1] != '\\')
		strcat(m_szInitDir, "\\");

	return true;
}

vector<string> CBrowseDir::BeginBrowseFilenames(const char* filespec)
{
	ProcessDir(m_szInitDir, NULL);
	return GetDirFilenames(m_szInitDir, filespec);
}

bool CBrowseDir::BeginBrowse(const char* filespec)
{
	ProcessDir(m_szInitDir, NULL);
	return BrowseDir(m_szInitDir, filespec);
}

bool CBrowseDir::BrowseDir(const char* dir, const char* filespec)
{
	_chdir(dir);

	//首先查找dir中符合要求的文件
	long hFile;
	_finddata_t fileinfo;
	if ((hFile = _findfirst(filespec, &fileinfo)) != -1)
	{
		do
		{
			//检查是不是目录
			//如果不是,则进行处理
			if (!(fileinfo.attrib & _A_SUBDIR))
			{
				char filename[_MAX_PATH];
				strcpy(filename, dir);
				strcat(filename, fileinfo.name);
				cout << filename << endl;
				if (!ProcessFile(filename))
					return false;
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
	//查找dir中的子目录
	//因为在处理dir中的文件时，派生类的ProcessFile有可能改变了
	//当前目录，因此还要重新设置当前目录为dir。
	//执行过_findfirst后，可能系统记录下了相关信息，因此改变目录
	//对_findnext没有影响。
	_chdir(dir);
	if ((hFile = _findfirst("*.*", &fileinfo)) != -1)
	{
		do
		{
			//检查是不是目录
			//如果是,再检查是不是 . 或 .. 
			//如果不是,进行迭代
			if ((fileinfo.attrib & _A_SUBDIR))
			{
				if (strcmp(fileinfo.name, ".") != 0 && strcmp
				(fileinfo.name, "..") != 0)
				{
					char subdir[_MAX_PATH];
					strcpy(subdir, dir);
					strcat(subdir, fileinfo.name);
					strcat(subdir, "\\");
					ProcessDir(subdir, dir);
					if (!BrowseDir(subdir, filespec))
						return false;
				}
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
	return true;
}

vector<string> CBrowseDir::GetDirFilenames(const char* dir, const char* filespec)
{
	_chdir(dir);
	vector<string>filename_vector;
	filename_vector.clear();

	//首先查找dir中符合要求的文件
	long hFile;
	_finddata_t fileinfo;
	if ((hFile = _findfirst(filespec, &fileinfo)) != -1)
	{
		do
		{
			//检查是不是目录
			//如果不是,则进行处理
			if (!(fileinfo.attrib & _A_SUBDIR))
			{
				char filename[_MAX_PATH];
				strcpy(filename, dir);
				strcat(filename, fileinfo.name);
				filename_vector.push_back(filename);
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
	//查找dir中的子目录
	//因为在处理dir中的文件时，派生类的ProcessFile有可能改变了
	//当前目录，因此还要重新设置当前目录为dir。
	//执行过_findfirst后，可能系统记录下了相关信息，因此改变目录
	//对_findnext没有影响。
	_chdir(dir);
	if ((hFile = _findfirst("*.*", &fileinfo)) != -1)
	{
		do
		{
			//检查是不是目录
			//如果是,再检查是不是 . 或 .. 
			//如果不是,进行迭代
			if ((fileinfo.attrib & _A_SUBDIR))
			{
				if (strcmp(fileinfo.name, ".") != 0 && strcmp
				(fileinfo.name, "..") != 0)
				{
					char subdir[_MAX_PATH];
					strcpy(subdir, dir);
					strcat(subdir, fileinfo.name);
					strcat(subdir, "\\");
					ProcessDir(subdir, dir);
					vector<string>tmp = GetDirFilenames(subdir, filespec);
					for (vector<string>::iterator it = tmp.begin(); it < tmp.end(); it++)
					{
						filename_vector.push_back(*it);
					}
				}
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
	return filename_vector;
}

bool CBrowseDir::ProcessFile(const char* filename)
{
	return true;
}

void CBrowseDir::ProcessDir(const char
	* currentdir, const char* parentdir)
{
}

//从CBrowseDir派生出的子类，用来统计目录中的文件及子目录个数
class CStatDir :public CBrowseDir
{
protected:
	int m_nFileCount;   //保存文件个数
	int m_nSubdirCount; //保存子目录个数

public:
	//缺省构造器
	CStatDir()
	{
		//初始化数据成员m_nFileCount和m_nSubdirCount
		m_nFileCount = m_nSubdirCount = 0;
	}

	//返回文件个数
	int GetFileCount()
	{
		return m_nFileCount;
	}

	//返回子目录个数
	int GetSubdirCount()
	{
		//因为进入初始目录时，也会调用函数ProcessDir，
		//所以减1后才是真正的子目录个数。
		return m_nSubdirCount - 1;
	}

protected:
	//覆写虚函数ProcessFile，每调用一次，文件个数加1
	virtual bool ProcessFile(const char* filename)
	{
		m_nFileCount++;
		return CBrowseDir::ProcessFile(filename);
	}

	//覆写虚函数ProcessDir，每调用一次，子目录个数加1
	virtual void ProcessDir
	(const char* currentdir, const char* parentdir)
	{
		m_nSubdirCount++;
		CBrowseDir::ProcessDir(currentdir, parentdir);
	}
};

int Window(void) {

	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(BROWSEINFO));
	bi.hwndOwner = NULL;
	bi.pszDisplayName = szBuffer;
	bi.lpszTitle = _T("请选择原神安装路径:");
	bi.ulFlags = BIF_RETURNFSANCESTORS;
	LPITEMIDLIST idl = SHBrowseForFolder(&bi);
	if (NULL == idl)
	{
		return 0;
	}
	SHGetPathFromIDList(idl, szBuffer);
	return 1;
}

//wchar_t转char
char* UnicodeToAnsi(const wchar_t* szStr)
{
	int nLen = WideCharToMultiByte(CP_ACP, 0, szStr, -1, NULL, 0, NULL, NULL);
	if (nLen == 0)
	{
		return NULL;
	}
	char* pResult = new char[nLen];
	WideCharToMultiByte(CP_ACP, 0, szStr, -1, pResult, nLen, NULL, NULL);
	return pResult;
}

int main(void)
{
	//获取目录名
	char* buf = NULL;
	//printf("请输入要统计的目录名:");
	//gets_s(buf);
	Window();
	buf = UnicodeToAnsi(szBuffer);

	//构造类对象
	CStatDir statdir;

	//设置要遍历的目录
	if (!statdir.SetInitDir(buf))
	{
		puts("目录不存在。");
		return 0;
	}

	//开始遍历
	string GenshinPath;
	string::size_type m;

	vector<string>file_vec = statdir.BeginBrowseFilenames("*.*");
	for (vector<string>::const_iterator it = file_vec.begin(); it < file_vec.end(); ++it) {
		std::cout << *it << std::endl;
		string str = *it;
		m = str.find("Genshin Impact Game\\YuanShen_Data\\Persistent\\AssetBundles\\blocks");
		if (m != string::npos) {
			GenshinPath = str;
			break;
		}
	}

	if (m == string::npos) {
		cout << endl << "该目录非原神安装目录" << endl;
		system("PAUSE");
		return 0;
	}
	else {
		string::size_type num = GenshinPath.find("blocks");
		GenshinPath = GenshinPath.substr(0, num + 7);
		cout << endl << "目标目录为:" << GenshinPath << endl;

		//生成cmd命令,重命名文件夹;
		string dos0 = "ren \"" + GenshinPath + "00\" " + "01";
		string dos1 = "rd /s /q \"" + GenshinPath + "01\"";
		string path0 = GenshinPath + "01";
		string path1 = GenshinPath + "00";
		if (0 == access(path0.data(), 0))
			system(dos1.data());
		if (0 == access(path1.data(), 0))
			system(dos0.data());
		cout << endl << "Debug success! Enjoy your game~" << endl << endl;

		//生成dat文件,一键Debug,方便使用;
		TCHAR DesktopPath[MAX_PATH] = { 0 };
		SHGetSpecialFolderPath(0, DesktopPath, CSIDL_DESKTOPDIRECTORY, 0);
		string Desktop = UnicodeToAnsi(DesktopPath);
		Desktop = Desktop + "\\Debug.bat";
		ofstream outfile;
		outfile.open(Desktop, std::ios::out | std::ios::app);//生成空的批处理文件
		if (!outfile)
		{
			cerr << "Open file error! Creat Debug.bat fail!" << endl << endl;
			system("PAUSE");
			exit(-1);
		}
		outfile << dos0 << endl << dos1;//写入cmd指令
		cout << "Creat Debug.bat success!" << endl << endl;
		outfile.close();

		printf("程序成功执行,文件总数: %d\n", file_vec.size());
		system("PAUSE");

		return 1;
	}
	
}