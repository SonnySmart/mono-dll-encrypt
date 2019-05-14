// mono-dll-encrypt.cpp : 定义控制台应用程序的入口点。
//

#ifdef _WIN32
#include "stdafx.h"
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <dirent.h>
#include "xxtea.c"
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <cstring>        // for strcpy(), strcat()
#include <vector>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>

#define MAXPATH  512

using namespace std;

bool exist(const char *path);

bool isDir(const char *name);

void listFiles(const char * dir, vector<string> &file_list);

void string_replace( std::string &strBig, const std::string &strsrc, const std::string &strdst);

void read_buffer(const char *filename, void *&buffer, size_t &len);

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
    
	char buffer[MAXPATH] = { 0 }; 
	const char *cwd = getcwd(buffer, MAXPATH);

	const char *encrypt_key  = argv[1];
	string encrypt_path(string(cwd).append("/").append(argv[2]));
	string decrypt_path(string(cwd).append("/").append(argv[3]));

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
        void *buffer = NULL;
        size_t len = 0;
        read_buffer(encrypt_file.c_str(), buffer, len);
        
        if (!buffer)
            continue;

		// 加密文件
		xxtea_long ret_len = 0;
		void *encrypt_data = xxtea_encrypt((unsigned char *)buffer, (xxtea_long)len, (unsigned char *)encrypt_key, (xxtea_long)strlen(encrypt_key), &ret_len);
		if (!encrypt_data || ret_len <= 0)
			return -1;

		// 替换路径
		string_replace(encrypt_file, encrypt_path, decrypt_path);
		string decrypt_file(encrypt_file);
		string parent_dir = decrypt_file.substr(0, decrypt_file.rfind("/"));

		if (!exist(parent_dir.c_str()))
        {
#ifdef _WIN32
            system(string("md ").append(parent_dir).c_str());
#else
            system(string("mkdir ").append(parent_dir).c_str());
#endif
        }

		// 写入文件
        FILE *fp = NULL;
        if (!(fp = fopen(decrypt_file.c_str(), "w")))
            return -1;
        
        fwrite(encrypt_data, 1u, ret_len, fp);
        
        fclose(fp);

		printf("%s 加密成功\n",decrypt_file.c_str());

		free(encrypt_data);
		free(buffer);

		++encrypt_count;
	}

	if (encrypt_count == file_list.size())
		printf("加密完成 \n");

	return 0;
}

//是否存在
bool exist(const char* path)
{
    return ( (access( path, 0 )) != -1 );
}

bool isDir(const char *name)
{
#ifdef _WIN32
	return GetFileAttributesA(name)&FILE_ATTRIBUTE_DIRECTORY;
#else
    struct stat stbuf;
    
    if(stat(name, &stbuf) == -1){
        return false;
    }
    
    return ((stbuf.st_mode & S_IFMT) == S_IFDIR);
#endif
}

void listFiles(const char * dir, vector<string> &file_list)
{
#ifdef _WIN32
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
#else
    char name[MAXPATH];
    struct dirent *dp;
    DIR *dfd;
    
    if((dfd = opendir(dir)) == NULL){
        fprintf(stderr, "dirwalk: can't open %s\n", dir);
        return;
    }
    
    while((dp = readdir(dfd)) != NULL){ //读目录记录项
        if(strcmp(dp->d_name, ".") == 0 || strcmp(dp -> d_name, "..") == 0){
            continue;  //跳过当前目录以及父目录
        }
        
        sprintf(name, "%s/%s", dir, dp->d_name);
        
        if (isDir(name))
            listFiles(name, file_list);
        else
            file_list.push_back(name);
    }
#endif
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

void read_buffer(const char *filename, void *&buffer, size_t &len)
{
    if (!exist(filename))
        return;
    
    FILE *fp = NULL;
    if (!(fp = fopen(filename, "r")))
        return;
    
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    if (len <= 0)
    {
        fclose(fp);
        return;
    }
    
    fseek(fp, 0, SEEK_SET);
    void *buff = malloc(len);
    if (fread(buff, 1u, len, fp) <= 0)
    {
        fclose(fp);
        return;
    }
    
    buffer = buff;
    
    fclose(fp);
}

