#ifndef __INTERVAL_TIMER_H
#define __INTERVAL_TIMER_H

#include "RakNetTypes.h"

struct IntervalTimer
{
	void SetPeriod(RakNetTime period);
	bool UpdateInterval(RakNetTime elapsed);

	RakNetTime basePeriod, remaining;	
};

#endif