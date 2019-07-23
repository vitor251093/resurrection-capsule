#include "STDStringCompressor.h"
#include "RakNetDefines.h"

#if defined(_PS3)
// Alloca
#include <alloca.h>
#endif
#include <stdlib.h>

bool STDDecodeString( std::string *output, int maxCharsToWrite, RakNet::BitStream *input, int languageID )
{
	if (maxCharsToWrite <= 0)
	{
		output->clear();
		return true;
	}

	char *destinationBlock;
	bool out;

#ifndef _CONSOLE_1
	if (maxCharsToWrite < MAX_ALLOCA_STACK_ALLOCATION)
	{
		destinationBlock = (char*) alloca(maxCharsToWrite);
		out=stringCompressor->DecodeString(destinationBlock, maxCharsToWrite, input, languageID);
		*output=destinationBlock;
	}
	else
#endif
	{
		destinationBlock = new char [maxCharsToWrite];
		out=stringCompressor->DecodeString(destinationBlock, maxCharsToWrite, input, languageID);
		*output=destinationBlock;
		delete [] destinationBlock;
	}

	return out;
}
