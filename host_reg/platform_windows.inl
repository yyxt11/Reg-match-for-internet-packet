#include <process.h>
#include "cross_platform.h"
//#include "Logger.h"
#include <sys/timeb.h>
#include <time.h>
#include "Shlwapi.h"
#include <Shlwapi.h>

#pragma comment(lib , "Shlwapi.lib")

bool csMkdir(const char* szDir)
{
	return _mkdir(szDir);
}

unsigned int __stdcall WindowsThreadCallback(void* arg)
{
	std::pair<void*, ThreadFunc> *pData = (std::pair<void*, ThreadFunc>*)arg, data = *pData;

	delete pData;

	return data.second(data.first);
}

int csCreateThread(ThreadFunc func, void* arg)
{
	std::pair<void*, ThreadFunc>* pData = new std::pair<void*, ThreadFunc>;
	pData->first = arg;
	pData->second = func;

	int tid = 0;
	_beginthreadex(0, 0, WindowsThreadCallback, (void*)pData, 0, (unsigned int*)&tid);

	return tid;
}

int csWaitAllThread(long* tid, int num)
{
	return WaitForMultipleObjects(num, (const HANDLE*)tid, true, INFINITE);
}

std::string csGetMacFromAdapterName(const std::string& name)
{
	LPADAPTER    lpAdapter = 0;
	DWORD        dwErrorCode;
	bool        Status;

	std::string n = name;
	lpAdapter = PacketOpenAdapter(&n[0]);
	if (!lpAdapter || (lpAdapter->hFile == INVALID_HANDLE_VALUE))
	{
		dwErrorCode = GetLastError();
		printf("Unable to open the adapter, Error Code : %lx\n", dwErrorCode);

		return "";
	}

	PPACKET_OID_DATA  OidData;
	OidData = (PPACKET_OID_DATA)malloc(6 + sizeof(PACKET_OID_DATA));
	if (OidData == NULL)
	{
		printf("error allocating memory!\n");
		PacketCloseAdapter(lpAdapter);
		return "";
	}

	//
	// Retrieve the adapter MAC querying the NIC driver
	//

	OidData->Oid = 0x01010102/*OID_802_3_CURRENT_ADDRESS*/;

	OidData->Length = 6;
	ZeroMemory(OidData->Data, 6);

	Status = PacketRequest(lpAdapter, FALSE, OidData);
	if (Status)
	{
		char buf[512];
		sprintf(buf, "%.2x-%.2x-%.2x-%.2x-%.2x-%.2x",
			(PCHAR)(OidData->Data)[0],
			(PCHAR)(OidData->Data)[1],
			(PCHAR)(OidData->Data)[2],
			(PCHAR)(OidData->Data)[3],
			(PCHAR)(OidData->Data)[4],
			(PCHAR)(OidData->Data)[5]);

		return buf;
	}
	else
	{
		printf("error retrieving the MAC address of the adapter!\n");
	}

	free(OidData);
	PacketCloseAdapter(lpAdapter);

	return "";
}

void csSleep(int millisecond)
{
	Sleep(millisecond);
}

csLock::csLock()
{
	InitializeCriticalSection(&m_criticalSection);
}

csLock::~csLock()
{
	DeleteCriticalSection(&m_criticalSection);
}

void csLock::Lock()
{
	EnterCriticalSection(&m_criticalSection);
}

void csLock::Unlock()
{
	LeaveCriticalSection(&m_criticalSection);
}

csLock::csLock(const csLock& rhs)
{
	InitializeCriticalSection(&m_criticalSection);
}

csLock& csLock::operator = (const csLock& rhs)
{
	return *this;
}

csConditionVariable::csConditionVariable()
{
	InitializeConditionVariable(&cv);
}

void csConditionVariable::Wait(csLock& lock)
{
	SleepConditionVariableCS(&cv, &lock.m_criticalSection, INFINITE);
}

void csConditionVariable::Signal()
{
	WakeConditionVariable(&cv);
}


char * csItoa(unsigned long val, char *buf, unsigned radix)
{
	return _itoa(val, buf, radix);
}

std::string csGetExePath(bool bWithFileName)
{
	char filename[MAX_PATH];
	GetModuleFileNameA(NULL, filename, sizeof(filename));

	if (!bWithFileName)
	PathAppendA(filename, "..\\");

	return filename;
}

int csGetCPUCount()
{
	SYSTEM_INFO  sysInfo;

	GetSystemInfo(&sysInfo);

	return sysInfo.dwNumberOfProcessors;
}

void csAtomicInc(volatile int* v)
{
	InterlockedIncrement((volatile unsigned int*)v);
}

void csAtomicDec(volatile int* v)
{
	InterlockedDecrement((volatile unsigned int*)v);
}

static HMODULE hModule = LoadLibraryA("kernel32.dll");
typedef VOID(WINAPI *PFN_GetSystemTimePreciseAsFileTime)(LPFILETIME lpSystemTimeAsFileTime);
static PFN_GetSystemTimePreciseAsFileTime pfnGetSystemTimePreciseAsFileTime = (PFN_GetSystemTimePreciseAsFileTime)GetProcAddress(hModule, "GetSystemTimePreciseAsFileTime");

timeval csGetTimeVal()
{
	FILETIME ft;
	if (pfnGetSystemTimePreciseAsFileTime)
	{
		pfnGetSystemTimePreciseAsFileTime(&ft);
	}
	else
	{
		GetSystemTimeAsFileTime(&ft);
	}

	LARGE_INTEGER SystemTime = (LARGE_INTEGER&)ft;

	timeval tv;
	tv.tv_sec = (LONG)(SystemTime.QuadPart / 10000000 - 11644473600);

	tv.tv_usec = (LONG)((SystemTime.QuadPart % 10000000) / 10);

	return tv;
}

int csSemaphore::Create(const char* name, bool bUseOpen)
{
	m_hSemaphore = CreateSemaphoreExA(NULL, 0, 1, name, 0, SEMAPHORE_ALL_ACCESS);
	if (NULL == m_hSemaphore)
	{
		return -1;
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		return 1;
	}

	return 0;
}

csSemaphore::~csSemaphore()
{
	CloseHandle(m_hSemaphore);
}

int csSemaphore::Wait()
{
	return WaitForSingleObject(m_hSemaphore, INFINITE);
}

int csSemaphore::Post()
{
	LONG preCount;
	return ReleaseSemaphore(m_hSemaphore, 1, &preCount);
}

int csCreateProcess(const char* path, char* arg)
{
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	std::string strArgs;
	strArgs += csGetExePath(true);
	strArgs += " ";
	strArgs += arg;

	BOOL ret = CreateProcessA(path, &strArgs[0], NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
	if (ret == 0) return -1;

	// Close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return pi.dwProcessId;
}

bool csDestroySemaphore(const char* name)
{
	return true;
}


csTlsObj::csTlsObj()
{
	m_tlsIdx = TlsAlloc();
}

bool csTlsObj::SetValue(void* tlsValue)
{
	return TlsSetValue(m_tlsIdx, tlsValue);
}

void* csTlsObj::GetValue()
{
	return TlsGetValue(m_tlsIdx);
}

int csGetThreadID()
{
	return (int)GetCurrentThreadId();
}