#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <boost/algorithm/string.hpp>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include "Logger.h"

#define gettid() syscall(__NR_gettid)

bool csMkdir(const char* szDir)
{
	return mkdir(szDir, S_IREAD | S_IWRITE);
}

void* LinuxThreadCallback(void* arg)
{
	std::pair<void*, ThreadFunc> *pData = (std::pair<void*, ThreadFunc>*)arg, data = *pData;

	delete pData;

	data.second(data.first);
}

int csCreateThread(ThreadFunc func, void* arg)
{
	pthread_t ntid;

	std::pair<void*, ThreadFunc>* pData = new std::pair<void*, ThreadFunc>;
	pData->first = arg;
	pData->second = func;
	int err = pthread_create(&ntid, NULL, LinuxThreadCallback, (void*)pData);
	return ntid;
}

int csWaitAllThread(long* tid, int num)
{
	//long tid;
	//while( (wait(NULL)) )
	//{
	//	print("capture thread dead, tid=%d\n", tid)
	//}
}

std::string csGetMacFromAdapterName(const std::string& name)
{
	return name;
}

void csSleep(int millisecond)
{
	usleep(millisecond * 1000);
}

csLock::csLock()
{
	pthread_mutex_init(&m_mutex, 0);
}

csLock::~csLock()
{
	pthread_mutex_destroy(&m_mutex);
}

void csLock::Lock()
{
	pthread_mutex_lock(&m_mutex);
}

void csLock::Unlock()
{
	pthread_mutex_unlock(&m_mutex);
}

csLock::csLock(const csLock& rhs)
{ 
	pthread_mutex_init(&m_mutex, 0);
}

csLock& csLock::operator = (const csLock& rhs) 
{
	return *this;
}

csConditionVariable::csConditionVariable()
{
	pthread_cond_init(&cv, 0);
}

csConditionVariable::~csConditionVariable()
{
	pthread_cond_destroy(&cv);
}

void csConditionVariable::Wait(csLock& lock)
{
	pthread_cond_wait(&cv, &lock.m_mutex);
}

void csConditionVariable::Signal()
{
	pthread_cond_signal(&cv);
}

std::string csGetExePath(bool bWithFileName)
{
	char current_absolute_path[1024] = {0};
	//获取当前程序绝对路径 
	int read_bytes = readlink("/proc/self/exe", current_absolute_path, sizeof(current_absolute_path));
	if (read_bytes < 0 || read_bytes >= sizeof(current_absolute_path))
	{
		return "";
	}

	std::string strPath = current_absolute_path;

	if (!bWithFileName)
	{
		int pos = strPath.find_last_of("/");
		strPath.resize(pos+1);
	}

	return strPath;
}

int csGetCPUCount()
{
	int cpu_num = sysconf(_SC_NPROCESSORS_CONF);
	return cpu_num;
}

void csAtomicInc(volatile int* v)
{
	//atomic_inc((atomic_t*)v);
}

void csAtomicDec(volatile int* v)
{
	//atomic_dec((atomic_t*)v);
}

timeval csGetTimeVal()
{
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv;
}

int csSemaphore::Create(const char* name, bool bUseOpen)
{
	m_bUseOpen = bUseOpen;

	if (name) m_strName = name;

	if (bUseOpen)
	{
		std::string strName = csGetExePath() + name;
		int ret = open(strName.c_str(), O_CREAT | O_WRONLY | O_TRUNC);
		if (-1 == ret)
		{
			if( errno == EACCES )
			{
				return 1;
			}

			return -1;
		}

		flock lock = { 0 };
		lock.l_type = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start = 0;
		lock.l_len = 0;

		int res = fcntl(ret, F_SETLK, &lock);
		if (-1 == res)
		{
			if (errno == EACCES || errno == EAGAIN)
			{
				return 1;
			}

			return -1;
		}

		m_hSemaphore = (sem_t*)ret;

		return 0;
	}
	else
	{
		if (name)
		{
			sem_t* ret = sem_open(name, O_CREAT | O_EXCL, S_IRWXU, 0);
			if (SEM_FAILED == ret)
			{
				if (errno == EEXIST)
				{
					ret = sem_open(name, O_CREAT, S_IRWXU, 0);
					if (SEM_FAILED == ret)
					{
						return -1;
					}

					m_hSemaphore = ret;
					return 1;
				}

				return -1;
			}

			m_hSemaphore = ret;
		}
		else
		{
			m_hSemaphore = new sem_t;
			if (sem_init(m_hSemaphore, 0, 0))
			{
				delete m_hSemaphore;
				return -1;
			}
		}
	}

	return 0;
}

csSemaphore::~csSemaphore()
{
	if (m_bUseOpen)
	{
		close((int)(long)m_hSemaphore);
	}
	else
	{
		if (m_hSemaphore)
		{
			if (!m_strName.empty())
			{
				sem_close(m_hSemaphore);
			}
			else
			{
				delete m_hSemaphore;
			}
		}
	}
}

int csSemaphore::Wait()
{
	if (m_bUseOpen) return 0;
	return sem_wait(m_hSemaphore);
}

int csSemaphore::Post()
{
	if (m_bUseOpen) return 0;
	return sem_post(m_hSemaphore);
}

int csCreateProcess(const char* path, char* arg)
{
	std::string strFile;
	std::string strPath;
	boost::iterator_range<const char*> res = boost::find_last(path, "/");
	if (!res.empty())
	{
		strFile = ".";
		strFile += res.begin();

		strPath.assign(path, res.begin());
	}
	else
	{
		strFile = path;
	}

	char old_cwd[1024];
	getcwd(old_cwd, sizeof(old_cwd));

	if (!strPath.empty())
		chdir(strPath.c_str());

	std::string cmd = strFile + " " + arg;
	system(cmd.c_str());

	if (!strPath.empty())
		chdir(old_cwd);

	return 0;

	signal(SIGCHLD, SIG_IGN);

	pid_t pid = fork();
	int status;

	if (pid < 0)
	{
		status = -1;
	}
	else if (pid == 0)
	{
		std::vector<std::string> vecStrArg;
		std::string strArg(arg);
		boost::split(vecStrArg, strArg, boost::is_any_of(" "));

		vecStrArg.push_back(vecStrArg[0]);
		vecStrArg[0] = path;

		std::vector<char*> vecArg;
		for (int i = 0; i < vecStrArg.size(); ++i)
		{
			vecArg.push_back((char*)vecStrArg[i].data());
		}
		vecArg.push_back(NULL);

		execv(strFile.c_str(), &vecArg[0]);

		_exit(127); //子进程正常执行则不会执行此语句
	}
	else
	{
		waitpid(pid, &status, WNOHANG | WUNTRACED);
	}

	return pid;
}

bool csDestroySemaphore(const char* name)
{
	return !sem_unlink(name);
}

void TlsDest(void *value)
{
}

csTlsObj::csTlsObj()
{
	pthread_key_create(&m_tlsIdx, TlsDest);
}

bool csTlsObj::SetValue(void* tlsValue)
{
	return pthread_setspecific(m_tlsIdx, tlsValue);
}

void* csTlsObj::GetValue()
{
	return pthread_getspecific(m_tlsIdx);
}

int csGetThreadID()
{
	return (int)pthread_self();
}


// 将数字转换为字符串
char * csItoa(unsigned long val, char *buf, unsigned radix)
{
	char *p;								/* pointer to traverse string */
	char *firstdig;							/* pointer to first digit */
	char temp;								/* temp char */
	unsigned digval;						/* value of digit */

	p = buf;
	firstdig = p;							/* save pointer to first digit */

	do{
		digval = (unsigned)(val % radix);
		val /= radix;										/* get next digit */

		/* convert to ascii and store */
		if (digval > 9)
			*p++ = (char)(digval - 10 + 'a');				/* a letter */
		else
			*p++ = (char)(digval + '0');					/* a digit */
	} while (val > 0);

	/* We now have the digit of the number in the buffer, but in reverse
	order. Thus we reverse them now. */

	*p-- = '\0 ';							/* terminate string; p points to last digit */

	do {
		temp = *p;
		*p = *firstdig;
		*firstdig = temp;					/* swap *p and *firstdig */
		--p;
		++firstdig;							/* advance to next two digits */
	} while (firstdig < p);					/* repeat until halfway */

	return buf;
}
