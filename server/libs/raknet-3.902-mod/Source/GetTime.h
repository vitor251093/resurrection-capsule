/// \file GetTime.h
/// \brief Returns the value from QueryPerformanceCounter.  This is the function RakNet uses to represent time.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __GET_TIME_H
#define __GET_TIME_H

#include "Export.h"
#include "RakNetTime.h" // For RakNetTime

/// The namespace RakNet is not consistently used.  It's only purpose is to avoid compiler errors for classes whose names are very common.
/// For the most part I've tried to avoid this simply by using names very likely to be unique for my classes.
namespace RakNet
{
	/// \brief Returns the value from QueryPerformanceCounter.  This is the function RakNet uses to represent time.
	/// Should be renamed GetTimeMS
	/// \note This time won't match the time returned by GetTimeCount(). See http://www.jenkinssoftware.com/forum/index.php?topic=2798.0
	RakNetTime RAK_DLL_EXPORT GetTime( void );

	/// Should be renamed GetTimeUS
	/// \note The maximum delta between returned calls is 1 second - however, RakNet calls this constantly anyway. See NormalizeTime() in the cpp.
	RakNetTimeUS RAK_DLL_EXPORT GetTimeNS( void );

	// Renames, for RakNet 4
	inline RakNetTime RAK_DLL_EXPORT GetTimeMS( void ) {return GetTime();}
	inline RakNetTimeUS RAK_DLL_EXPORT GetTimeUS( void ) {return GetTimeNS();}
}

#endif
