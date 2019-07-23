#include "IntervalTimer.h"

void IntervalTimer::SetPeriod(RakNetTime period) {basePeriod=period; remaining=0;}
bool IntervalTimer::UpdateInterval(RakNetTime elapsed)
{
	if (elapsed >= remaining)
	{
		RakNetTime difference = elapsed-remaining;
		if (difference >= basePeriod)
		{
			remaining=basePeriod;
		}
		else
		{
			remaining=basePeriod-difference;
		}

		return true;
	}

	remaining-=elapsed;
	return false;
}