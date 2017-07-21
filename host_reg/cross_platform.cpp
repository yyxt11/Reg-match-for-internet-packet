#include "cross_platform.h"

#ifdef WIN32///windowsƽ
	#include "platform_windows.inl"
#else///linux
	#include "platform_linux.inl"
#endif


clock_t csGetProcessElapseTime()
{
	clock_t unit = clock();
	clock_t cur_time = unit * (1000.f / CLOCKS_PER_SEC);
	return cur_time;
}

long TimeValDifference(const timeval& tv1, const timeval& tv2, long* microsecond)
{
	timeval res;
	res.tv_sec = tv2.tv_sec - tv1.tv_sec;
	res.tv_usec = tv2.tv_usec - tv1.tv_usec;

	if (microsecond)
	{
		*microsecond = res.tv_sec * 1000000 + res.tv_usec;
	}

	return res.tv_sec * 1000 + res.tv_usec / 1000;
}


