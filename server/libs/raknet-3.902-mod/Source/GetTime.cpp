/// \file
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#if defined(_WIN32) 
#include "WindowsIncludes.h"
// To call timeGetTime
// on Code::Blocks, this needs to be libwinmm.a instead
#pragma comment(lib, "Winmm.lib")
#endif

#include "GetTime.h"



#if defined(_WIN32)
DWORD mProcMask;
DWORD mSysMask;
HANDLE mThread;







#else
#include <sys/time.h>
#include <unistd.h>
static timeval tp;
RakNetTimeUS initialTime;
#endif

static bool initialized=false;

#if defined(GET_TIME_SPIKE_LIMIT) && GET_TIME_SPIKE_LIMIT>0
#include "SimpleMutex.h"
RakNetTimeUS lastNormalizedReturnedValue=0;
RakNetTimeUS lastNormalizedInputValue=0;
/// This constraints timer forward jumps to 1 second, and does not let it jump backwards
/// See http://support.microsoft.com/kb/274323 where the timer can sometimes jump forward by hours or days
/// This also has the effect where debugging a sending system won't treat the time spent halted past 1 second as elapsed network time
RakNetTimeUS NormalizeTime(RakNetTimeUS timeIn)
{
	RakNetTimeUS diff, lastNormalizedReturnedValueCopy;
	static SimpleMutex mutex;
	
	mutex.Lock();
	if (timeIn>=lastNormalizedInputValue)
	{
		diff = timeIn-lastNormalizedInputValue;
		if (diff > GET_TIME_SPIKE_LIMIT)
			lastNormalizedReturnedValue+=GET_TIME_SPIKE_LIMIT;
		else
			lastNormalizedReturnedValue+=diff;
	}
	else
		lastNormalizedReturnedValue+=GET_TIME_SPIKE_LIMIT;

	lastNormalizedInputValue=timeIn;
	lastNormalizedReturnedValueCopy=lastNormalizedReturnedValue;
	mutex.Unlock();

	return lastNormalizedReturnedValueCopy;
}
#endif // #if defined(GET_TIME_SPIKE_LIMIT) && GET_TIME_SPIKE_LIMIT>0
RakNetTime RakNet::GetTime( void )
{
	return (RakNetTime)(GetTimeNS()/1000);
}










































#if   defined(_WIN32) 
RakNetTimeUS GetTimeUS_Windows( void )
{
	if ( initialized == false)
	{
		initialized = true;

		// Save the current process
#if !defined(_WIN32_WCE)
		HANDLE mProc = GetCurrentProcess();

		// Get the current Affinity
#if _MSC_VER >= 1400 && defined (_M_X64)
		GetProcessAffinityMask(mProc, (PDWORD_PTR)&mProcMask, (PDWORD_PTR)&mSysMask);
#else
		GetProcessAffinityMask(mProc, &mProcMask, &mSysMask);
#endif
		mThread = GetCurrentThread();

#endif // _WIN32_WCE
	}	

	// 9/26/2010 In China running LuDaShi, QueryPerformanceFrequency has to be called every time because CPU clock speeds can be different
	RakNetTimeUS curTime;
	LARGE_INTEGER PerfVal;
	LARGE_INTEGER yo1;

	QueryPerformanceFrequency( &yo1 );
	QueryPerformanceCounter( &PerfVal );

	__int64 quotient, remainder;
	quotient=((PerfVal.QuadPart) / yo1.QuadPart);
	remainder=((PerfVal.QuadPart) % yo1.QuadPart);
	curTime = (RakNetTimeUS) quotient*(RakNetTimeUS)1000000 + (remainder*(RakNetTimeUS)1000000 / yo1.QuadPart);

#if defined(GET_TIME_SPIKE_LIMIT) && GET_TIME_SPIKE_LIMIT>0
	return NormalizeTime(curTime);
#else
	return curTime;
#endif // #if defined(GET_TIME_SPIKE_LIMIT) && GET_TIME_SPIKE_LIMIT>0
}
#elif (defined(__GNUC__)  || defined(__GCCXML__))
RakNetTimeUS GetTimeUS_Linux( void )
{
	if ( initialized == false)
	{
		gettimeofday( &tp, 0 );
		initialized=true;
		// I do this because otherwise RakNetTime in milliseconds won't work as it will underflow when dividing by 1000 to do the conversion
		initialTime = ( tp.tv_sec ) * (RakNetTimeUS) 1000000 + ( tp.tv_usec );
	}

	// GCC
	RakNetTimeUS curTime;
	gettimeofday( &tp, 0 );

	curTime = ( tp.tv_sec ) * (RakNetTimeUS) 1000000 + ( tp.tv_usec );

#if defined(GET_TIME_SPIKE_LIMIT) && GET_TIME_SPIKE_LIMIT>0
	return NormalizeTime(curTime - initialTime);
#else
	return curTime - initialTime;
#endif // #if defined(GET_TIME_SPIKE_LIMIT) && GET_TIME_SPIKE_LIMIT>0
}
#endif

RakNetTimeUS RakNet::GetTimeNS( void )
{




#if   defined(_WIN32)
	return GetTimeUS_Windows();
#else
	return GetTimeUS_Linux();
#endif
}
