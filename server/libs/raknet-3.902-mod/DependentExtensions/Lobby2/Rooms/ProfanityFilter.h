#ifndef __PROFANITY_FILTER__H__
#define __PROFANITY_FILTER__H__

#include "DS_List.h"
#include "RakString.h"

class ProfanityFilter
{
public:
	ProfanityFilter();
	~ProfanityFilter();

	// Returns true if the string has profanity, false if not.
	bool HasProfanity(const char *str);

	// Removes profanity. Returns number of occurrences of profanity matches (including 0)
	int FilterProfanity(const char *str, char *output, bool filter = true); 		
	
	// Number of profanity words loaded
	int Count();

	void AddWord(RakNet::RakString newWord);
private:	
	DataStructures::List<RakNet::RakString> words;

	char RandomBanChar();

	static char BANCHARS[];
	static char WORDCHARS[];
};

#endif // __PROFANITY__H__
