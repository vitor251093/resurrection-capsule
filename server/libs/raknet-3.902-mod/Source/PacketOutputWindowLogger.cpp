#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_PacketLogger==1

#include "PacketOutputWindowLogger.h"
#include "RakString.h"
#if defined(_WIN32) && !defined(__X360__) 
#include "WindowsIncludes.h"
#endif

PacketOutputWindowLogger::PacketOutputWindowLogger()
{
}
PacketOutputWindowLogger::~PacketOutputWindowLogger()
{
}
void PacketOutputWindowLogger::WriteLog(const char *str)
{
#if defined(_WIN32) && !defined(__X360__) 
	RakNet::RakString str2 = str;
	str2+="\n";
	OutputDebugString(str2.C_String());
#endif
}

#endif // _RAKNET_SUPPORT_*
