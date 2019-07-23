/// \file
/// \brief Writes RakNetStatistics for all connected systems once per second to SQLiteClientLogger
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __SQL_LITE_CLIENT_LOGGER_RAKNET_STATISTICS_H_
#define __SQL_LITE_CLIENT_LOGGER_RAKNET_STATISTICS_H_

#include "PluginInterface2.h"

namespace RakNet
{
	/// \ingroup PACKETLOGGER_GROUP
	/// \brief Packetlogger that outputs to a file
	class RAK_DLL_EXPORT SQLiteClientLogger_RakNetStatistics : public PluginInterface2
	{
	public:
		SQLiteClientLogger_RakNetStatistics();
		virtual ~SQLiteClientLogger_RakNetStatistics();
		virtual void Update(void);
	protected:
		RakNetTimeUS lastUpdate;
	};
}

#endif
