// mono-dll-encrypt.cpp : �������̨Ӧ�ó������ڵ㡣
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

//�Ƿ����
bool exist(const char * lpPath);

//�Ƿ����ļ��� #include <Windows.h>
bool isDir(const char *lpPath);

void listFiles(const char * dir, vector<string> &file_list);

void string_replace( std::string &strBig, const std::string &strsrc, const std::string &strdst);

int main(int argc, char* argv[])
{
	if (argc < 4)
	{
		printf("������������ \n");
		return -1;
	}

	printf("���ܳ��� = %s \n", *(argv + 0));
	printf("����key  = %s \n", *(argv + 1));
	printf("����·�� = %s \n", *(argv + 2));
	printf("���·�� = %s \n", *(argv + 3));
    
	xxtea_long data_len;
	char buffer[MAXPATH] = { 0 }; 
	const char *cwd = _getcwd(buffer, MAXPATH); 

	const char *encrypt_key  = argv[1];
	string encrypt_path(string(cwd).append("\\").append(argv[2]));
	string decrypt_path(string(cwd).append("\\").append(argv[3]));

	vector<string> file_list;

	// ƴ��ȫ·��
	printf("%s \n",encrypt_path.c_str());
	
	// �ж��ļ��Ƿ����
	if (!exist(encrypt_path.c_str()))
	{
		printf("�ļ������� \n");
		return -1;
	}

	// ����ļ�·�����б�
	if (isDir(encrypt_path.c_str()))
	{
		listFiles(encrypt_path.c_str(), file_list);
	}
	else
	{
		file_list.push_back(encrypt_path);
	}

	// �ж�Ϊ���ļ����˳�
	if (file_list.empty())
		return -1;

	size_t encrypt_count(0);

	// �����ļ���
	for(size_t i = 0; i < file_list.size(); i++)
	{
		string encrypt_file(file_list.at(i));

		if (!exist(encrypt_file.c_str()))
			continue;

		// ���ļ�
		fstream fs(file_list.at(i), ios::in | ios::binary);
		if (!fs.is_open())
			return -1;

		fs.seekg(0, fs.end);
		data_len = fs.tellg();

		void *data = malloc(data_len);

		// ��ȡ�ļ�
		fs.seekg(0, fs.beg);
		if (fs.read((char *)data, data_len) < 0)
		{
			printf("fread error \n");
			return -1;
		}

		// �����ļ�
		xxtea_long ret_len;
		void *encrypt_data = xxtea_encrypt((unsigned char *)data, data_len, (unsigned char *)encrypt_key, (xxtea_long)strlen(encrypt_key), &ret_len);
		if (!encrypt_data)
			return -1;

		// base64
		//char * base64_data = base64_encode((const unsigned char *)encrypt_data, len);
		// �滻·��
		string_replace(encrypt_file, encrypt_path, decrypt_path);
		string decrypt_file(encrypt_file);
		string parent_dir = decrypt_file.substr(0, decrypt_file.rfind("\\"));

		if (!exist(parent_dir.c_str()))
			system(string("md ").append(parent_dir).c_str());

		fstream fp(decrypt_file, ios::out | ios::binary);
		if (!fp.is_open())
			return -1;

		// д���ļ�
		fp.seekp(0, fp.beg);
		if (fp.write((const char *)encrypt_data, ret_len) < 0)
		{
			printf("fwrite error \n");
			return -1;
		}

		printf("%s ���ܳɹ�\n",decrypt_file.c_str());

		free(encrypt_data);
		free(data);
		fs.close();
		fp.close();

		++encrypt_count;
	}

	if (encrypt_count == file_list.size())
		printf("������� \n");

	return 0;
}

//�Ƿ����
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

//�Ƿ����ļ��� #include <Windows.h>
bool isDir(const char *lpPath)
{
	return GetFileAttributesA(lpPath)&FILE_ATTRIBUTE_DIRECTORY;
}

void listFiles(const char * dir, vector<string> &file_list)
{
	char dirNew[200];
	strcpy(dirNew, dir);
	strcat(dirNew, "\\*.*");    // ��Ŀ¼�������"\\*.*"���е�һ������

	intptr_t handle;
	_finddata_t findData;

	handle = _findfirst(dirNew, &findData);
	if (handle == -1)        // ����Ƿ�ɹ�
		return;

	do
	{
		if (findData.attrib & _A_SUBDIR)
		{
			if (strcmp(findData.name, ".") == 0 || strcmp(findData.name, "..") == 0)
				continue;

			//cout << findData.name << "\t<dir>\n";

			// ��Ŀ¼�������"\\"����������Ŀ¼��������һ������
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

	_findclose(handle);    // �ر��������
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

