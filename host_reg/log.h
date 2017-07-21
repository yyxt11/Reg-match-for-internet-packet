/**
* �������log�ļ�����.
*/


#ifndef LOG_H    
#define LOG_H    

#define filesize 1024*1024*50

//log�ļ�·��  
#define LOG_FILE_NAME "log.txt"  

//���ÿ���  
#define LOG_ENABLE  

#include <fstream>    
#include <string>    
#include <ctime>    
#include <stdio.h>

using namespace std;

BOOL LOG_COUNTINUE = TRUE;



class CLog
{
public:
	
	static void GetLogFilePath(CHAR* szPath)
	{
		GetModuleFileNameA(NULL, szPath, MAX_PATH);
		ZeroMemory(strrchr(szPath, _T('\\')), strlen(strrchr(szPath, _T('\\')))*sizeof(CHAR));
		strcat(szPath, "\\");
		strcat(szPath, LOG_FILE_NAME);
	}


	static void ResetLogFile()
	{
		CHAR szPath[MAX_PATH] = { 0 };
		GetLogFilePath(szPath);
		if (remove(szPath) == -1)
			return;
	}

	//���һ�����ݣ��������ַ���(ascii)����������������������ö��  
	//��ʽΪ��[2011-11-11 11:11:11] aaaaaaa������  
	template <class T>
	static void WriteLog(T x)
	{
		if (LOG_COUNTINUE)
		{
			CHAR szPath[MAX_PATH] = { 0 };
			GetLogFilePath(szPath);

			ofstream fout(szPath, ios::app);
			fout.seekp(ios::end);
			fout << GetSystemTime() << x << endl;
			fout.close();
			ifstream in(szPath);
			in.seekg(0, ios::end);      //�����ļ�ָ�뵽�ļ�����β��
			streampos ps = in.tellg();  //��ȡ�ļ�ָ���λ��
			if (ps >= filesize)
				LOG_COUNTINUE = FALSE;
			else
				LOG_COUNTINUE = TRUE;
		}
	}

	//���2�����ݣ��ԵȺ����ӡ�һ������ǰ����һ�������������ַ�������������������ֵ  
	template<class T1, class T2>
	static void WriteLog2(T1 x1, T2 x2)
	{
		if (LOG_COUNTINUE)
		{
			CHAR szPath[MAX_PATH] = { 0 };
			GetLogFilePath(szPath);
			ofstream fout(szPath, ios::app);
			fout.seekp(ios::end);
			fout << GetSystemTime() << x1 << " = " << x2 << endl;
			fout.close();
			ifstream in(szPath);
			in.seekg(0, ios::end);      //�����ļ�ָ�뵽�ļ�����β��
			streampos ps = in.tellg();  //��ȡ�ļ�ָ���λ��
			if (ps >= filesize)
				LOG_COUNTINUE = FALSE;
			else
				LOG_COUNTINUE = TRUE;
		}
	}

	//���һ�е�ǰ������ʼ�ı�־,�괫��__FUNCTION__  
	template <class T>
	static void WriteFuncBegin(T x)
	{
		if (LOG_COUNTINUE)
		{
			CHAR szPath[MAX_PATH] = { 0 };
			GetLogFilePath(szPath);
			ofstream fout(szPath, ios::app);
			fout.seekp(ios::end);
			fout << GetSystemTime() << "    --------------------" << x << "  Begin--------------------" << endl;
			fout.close();
			ifstream in(szPath);
			in.seekg(0, ios::end);      //�����ļ�ָ�뵽�ļ�����β��
			streampos ps = in.tellg();  //��ȡ�ļ�ָ���λ��
			if (ps >= filesize)
				LOG_COUNTINUE = FALSE;
			else
				LOG_COUNTINUE = TRUE;
		}
	}

	//���һ�е�ǰ���������ı�־���괫��__FUNCTION__  
	template <class T>
	static void WriteFuncEnd(T x)
	{
		if (LOG_COUNTINUE)
		{

			CHAR szPath[MAX_PATH] = { 0 };
			GetLogFilePath(szPath);
			ofstream fout(szPath, ios::app);
			fout.seekp(ios::end);
			fout << GetSystemTime() << "--------------------" << x << "  End  --------------------" << endl;
			fout.close();
			ifstream in(szPath);
			in.seekg(0, ios::end);      //�����ļ�ָ�뵽�ļ�����β��
			streampos ps = in.tellg();  //��ȡ�ļ�ָ���λ��
			if (ps >= filesize)
				LOG_COUNTINUE = FALSE;
			else
				LOG_COUNTINUE = TRUE;
		}
	}


public:
	//��ȡ����ʱ�䣬��ʽ��"[2011-11-11 11:11:11] ";   
	static string GetSystemTime()
	{
		time_t tNowTime;
		time(&tNowTime);
		tm* tLocalTime = localtime(&tNowTime);
		char szTime[30] = { '\0' };
		strftime(szTime, 30, "[%Y-%m-%d %H:%M:%S] ", tLocalTime);
		string strTime = szTime;
		return strTime;
	}

};

#endif


#ifdef LOG_ENABLE
//��������Щ����ʹ�ñ��ļ�  
#define LOG(x)              CLog::WriteLog(x);          //�����ڿ������ַ���(ascii)����������������bool��  
#define LOG2(x1,x2)     CLog::WriteLog2(x1,x2);  
#define LOG_FUNC        LOG(__FUNCTION__)               //�����ǰ���ں�����  
#define LOG_LINE        LOG(__LINE__)                       //�����ǰ�к�  
#define LOG_FUNC_BEGIN  CLog::WriteFuncBegin(__FUNCTION__);     //��ʽ�磺[ʱ��]"------------FuncName  Begin------------"  
#define LOG_FUNC_END     CLog::WriteFuncEnd(__FUNCTION__);      //��ʽ�磺[ʱ��]"------------FuncName  End------------"  
#else
#define LOG(x)
#define LOG2(x1, x2)
#define LOG_FUNC        
#define LOG_LINE       
#define LOG_FUNC_BEGIN  
#define LOG_FUNC_END     
#endif