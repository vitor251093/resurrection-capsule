#ifndef __STD_STRING_COMPRESSOR_H
#define __STD_STRING_COMPRESSOR_H

#include "StringCompressor.h"
#include <string>

bool STDDecodeString( std::string *output, int maxCharsToWrite, RakNet::BitStream *input, int languageID=0 );

#endif
