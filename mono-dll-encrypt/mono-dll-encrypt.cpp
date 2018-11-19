// mono-dll-encrypt.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "xxtea.c"
#include <string.h>
#include <stdio.h>
#include <direct.h>
#include <Windows.h>
#include <iostream>
#include <cstring>        // for strcpy(), strcat()
#include <io.h>
#include <vector>
#include <fstream>

#define MAXPATH  1024 

using namespace std;

//是否存在
bool exist(const char * lpPath);

//是否是文件夹 #include <Windows.h>
bool isDir(const char *lpPath);

void listFiles(const char * dir, vector<string> &file_list);

void string_replace( std::string &strBig, const std::string &strsrc, const std::string &strdst);

int main(int argc, char* argv[])
{
	if (argc < 4)
	{
		printf("参数个数错误 \n");
		return -1;
	}

	printf("加密程序 = %s \n", *(argv + 0));
	printf("加密key  = %s \n", *(argv + 1));
	printf("加密路径 = %s \n", *(argv + 2));
	printf("输出路径 = %s \n", *(argv + 3));
    
	xxtea_long data_len;
	char buffer[MAXPATH] = { 0 }; 
	const char *cwd = _getcwd(buffer, MAXPATH); 

	const char *encrypt_key  = argv[1];
	string encrypt_path(string(cwd).append("\\").append(argv[2]));
	string decrypt_path(string(cwd).append("\\").append(argv[3]));

	vector<string> file_list;

	// 拼接全路径
	printf("%s \n",encrypt_path.c_str());
	
	// 判断文件是否存在
	if (!exist(encrypt_path.c_str()))
	{
		printf("文件不存在 \n");
		return -1;
	}

	// 添加文件路径到列表
	if (isDir(encrypt_path.c_str()))
	{
		listFiles(encrypt_path.c_str(), file_list);
	}
	else
	{
		file_list.push_back(encrypt_path);
	}

	// 判断为空文件夹退出
	if (file_list.empty())
		return -1;

	size_t encrypt_count(0);

	// 遍历文件夹
	for(size_t i = 0; i < file_list.size(); i++)
	{
		string encrypt_file(file_list.at(i));

		if (!exist(encrypt_file.c_str()))
			continue;

		// 打开文件
		fstream fs(file_list.at(i), ios::in | ios::binary);
		if (!fs.is_open())
			return -1;

		fs.seekg(0, fs.end);
		data_len = fs.tellg();

		void *data = malloc(data_len);

		// 读取文件
		fs.seekg(0, fs.beg);
		if (fs.read((char *)data, data_len) < 0)
		{
			printf("fread error \n");
			return -1;
		}

		// 加密文件
		xxtea_long ret_len;
		void *encrypt_data = xxtea_encrypt((unsigned char *)data, data_len, (unsigned char *)encrypt_key, (xxtea_long)strlen(encrypt_key), &ret_len);
		if (!encrypt_data)
			return -1;

		// base64
		//char * base64_data = base64_encode((const unsigned char *)encrypt_data, len);
		// 替换路径
		string_replace(encrypt_file, encrypt_path, decrypt_path);
		string decrypt_file(encrypt_file);
		string parent_dir = decrypt_file.substr(0, decrypt_file.rfind("\\"));

		if (!exist(parent_dir.c_str()))
			system(string("md ").append(parent_dir).c_str());

		fstream fp(decrypt_file, ios::out | ios::binary);
		if (!fp.is_open())
			return -1;

		// 写入文件
		fp.seekp(0, fp.beg);
		if (fp.write((const char *)encrypt_data, ret_len) < 0)
		{
			printf("fwrite error \n");
			return -1;
		}

		printf("%s 加密成功\n",decrypt_file.c_str());

		free(encrypt_data);
		free(data);
		fs.close();
		fp.close();

		++encrypt_count;
	}

	if (encrypt_count == file_list.size())
		printf("加密完成 \n");

	return 0;
}

//是否存在
bool exist(const char * lpPath)
{
	/* Check for existence */
	if( (_access( lpPath, 0 )) != -1 )
	{
		return true;
	}else{
		return false;
	}
}

//是否是文件夹 #include <Windows.h>
bool isDir(const char *lpPath)
{
	return GetFileAttributesA(lpPath)&FILE_ATTRIBUTE_DIRECTORY;
}

void listFiles(const char * dir, vector<string> &file_list)
{
	char dirNew[200];
	strcpy(dirNew, dir);
	strcat(dirNew, "\\*.*");    // 在目录后面加上"\\*.*"进行第一次搜索

	intptr_t handle;
	_finddata_t findData;

	handle = _findfirst(dirNew, &findData);
	if (handle == -1)        // 检查是否成功
		return;

	do
	{
		if (findData.attrib & _A_SUBDIR)
		{
			if (strcmp(findData.name, ".") == 0 || strcmp(findData.name, "..") == 0)
				continue;

			//cout << findData.name << "\t<dir>\n";

			// 在目录后面加上"\\"和搜索到的目录名进行下一次搜索
			strcpy(dirNew, dir);
			strcat(dirNew, "\\");
			strcat(dirNew, findData.name);

			listFiles(dirNew, file_list);
		}
		else
		{
			//cout << findData.name << "\t" << findData.size << " bytes.\n";

			file_list.push_back(string(dir).append("\\").append(findData.name));
		}
	} while (_findnext(handle, &findData) == 0);

	_findclose(handle);    // 关闭搜索句柄
}

void string_replace( std::string &strBig, const std::string &strsrc, const std::string &strdst)
{
	std::string::size_type pos = 0;
	std::string::size_type srclen = strsrc.size();
	std::string::size_type dstlen = strdst.size();

	while( (pos=strBig.find(strsrc, pos)) != std::string::npos )
	{
		strBig.replace( pos, srclen, strdst );
		pos += dstlen;
	}
}

