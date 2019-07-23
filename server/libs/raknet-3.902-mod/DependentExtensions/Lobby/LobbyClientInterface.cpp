#include "LobbyClientInterface.h"
#include "EpochTimeToString.h"
#include <stdio.h>

using namespace RakNet;

void LobbyClientCBResult::DebugPrintf(void)
{
	char buff[4096];
	DebugSprintf(buff);
	printf("%s", buff);
}
void LobbyClientCBResult::DebugSprintf(char *output)
{
	sprintf(output, "%s\n", ErrorCodeDescription::ToEnglish(resultCode));
	if (additionalInfo && additionalInfo[0])
		sprintf(output+strlen(output), "additionalInfo: %s\n", additionalInfo);
	if (resultCode==LC_RC_SUSPENDED_USER_ID)
		sprintf(output+strlen(output), "Time: %s\n", EpochTimeToString(epochTime));
}


LC_Friend::LC_Friend()
{
	handle=0;
	id=0;
}
LC_Friend::~LC_Friend()
{
	if (handle)
		delete [] handle;
	if (id)
		delete id;
}

LC_Ignored::LC_Ignored()
{
	actionString=0;
	id=0;
}
LC_Ignored::~LC_Ignored()
{
	if (actionString)
		delete [] actionString;
	if (id)
		delete id;
}