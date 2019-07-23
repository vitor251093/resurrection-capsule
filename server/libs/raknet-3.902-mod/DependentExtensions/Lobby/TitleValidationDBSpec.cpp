#include "TitleValidationDBSpec.h"
#include "BitStream.h"
#include "StringCompressor.h"
#include "RakAssert.h"

using namespace TitleValidationDBSpec;

AddTitle_Data::AddTitle_Data()
{
	titlePassword=0;
	titlePasswordLength=0;
	
	requireUserKeyToLogon=false;
	allowClientAccountCreation=true;
	defaultAllowUpdateHandle=true;
	defaultAllowUpdateCCInfo=true;
	defaultAllowUpdateAccountNumber=true;
	defaultAllowUpdateAdminLevel=true;
	defaultAllowUpdateAccountBalance=true;
	defaultAllowClientsUploadActionHistory=true;
}

AddTitle_Data::~AddTitle_Data()
{
	if (titlePassword)
		delete [] titlePassword;
}

GetTitles_Data::GetTitles_Data()
{
}

GetTitles_Data::~GetTitles_Data()
{
	unsigned i;
	for (i=0; i < titles.Size(); i++)
	{
		titles[i]->Deref();
	}
}
	
UpdateUserKey_Data::UpdateUserKey_Data()
{
	keyPassword=0;
	keyPasswordLength=0;
	binaryData=0;
	binaryDataLength=0;
}

UpdateUserKey_Data::~UpdateUserKey_Data()
{
	if (keyPassword)
			delete [] keyPassword;
	if (binaryData)
		delete [] binaryData;
}
	
GetUserKeys_Data::GetUserKeys_Data()
{
}

GetUserKeys_Data::~GetUserKeys_Data()
{
	unsigned i;
	for (i=0; i < keys.Size(); i++)
	{
		keys[i]->Deref();
	}
}
ValidateUserKey_Data::ValidateUserKey_Data()
{
	keyPassword=0;
	keyPasswordLength=0;
	binaryData=0;
	binaryDataLength=0;
}
ValidateUserKey_Data::~ValidateUserKey_Data()
{
	if (keyPassword)
		delete [] keyPassword;
	if (binaryData)
		delete [] binaryData;
}