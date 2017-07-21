#pragma once
/*跨平台函数库*/
#include <string>
#include <ctime>

#ifdef WIN32

#include "Packet32.h"
#include <process.h>
#include <direct.h>

#define mkdir(a,b) mkdir(a)

class csLock
{
public:
	csLock();
	~csLock();

	csLock(const csLock& rhs);
	csLock& operator = (const csLock& rhs);

	void Lock();
	void Unlock();

private:
	friend class csConditionVariable;
	CRITICAL_SECTION m_criticalSection;
};

class csSemaphore
{
public:
	csSemaphore() :m_hSemaphore(NULL){}
	~csSemaphore();

	int Create(const char* name, bool bUseOpen = false);
	int Wait();
	int Post();

private:
	HANDLE m_hSemaphore;
};

class csConditionVariable
{
public:
	csConditionVariable();

	void Wait(csLock& lock);

	void Signal();

private:
	CONDITION_VARIABLE cv;
};

class csTlsObj
{
public:
	csTlsObj();
	bool SetValue(void* tlsValue);
	void* GetValue();

private:
	DWORD m_tlsIdx;
};

#else /// linux平台

#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>

#define MAX_PATH 260

class csLock
{
public:
	csLock();
	~csLock();

	void Lock();
	void Unlock();

	csLock(const csLock& rhs);
	csLock& operator = (const csLock& rhs);

private:
	friend class csConditionVariable;
	pthread_mutex_t m_mutex;
};

class csConditionVariable
{
public:
	csConditionVariable();

	~csConditionVariable();

	void Wait(csLock& lock);

	void Signal();

private:
	pthread_cond_t cv;
};

class csSemaphore
{
public:
	csSemaphore() :m_hSemaphore(0), m_bUseOpen(0){}
	~csSemaphore();

	int Create(const char* name,  bool bUseOpen = false);
	int Wait();
	int Post();
private:
	sem_t* m_hSemaphore;
	std::string m_strName;
	bool m_bUseOpen;
};

class csTlsObj
{
public:
	csTlsObj();
	bool SetValue(void* tlsValue);
	void* GetValue();

private:
	pthread_key_t  m_tlsIdx;
};

#endif

typedef unsigned int(*ThreadFunc) (void *);

int csCreateThread(ThreadFunc func, void* arg);
int csWaitAllThread(long* tid, int num);
std::string csGetMacFromAdapterName(const std::string& name);
void csSleep(int millisecond);
char * csItoa(unsigned long val, char *buf, unsigned radix);
std::string csGetExePath(bool bWithFileName = false);
bool csMkdir(const char* szDir);
int csGetCPUCount();
void csAtomicInc(volatile int* v);
void csAtomicDec(volatile int* v);

/// 返回进程自从启动逝去的毫秒数
clock_t csGetProcessElapseTime();
timeval csGetTimeVal();
/// tv2 与 tv1的差，毫秒数
long TimeValDifference(const timeval& tv1, const timeval& tv2, long* microsecond = NULL);

int csCreateProcess(const char* path, char* arg);

bool csDestroySemaphore(const char* name);
int csGetThreadID();
