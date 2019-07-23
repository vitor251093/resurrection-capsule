#include "RankingServerDBSpec.h"
#include "BitStream.h"
#include "StringCompressor.h"
#include "RakAssert.h"
#include "StringCompressor.h"

using namespace RankingServerDBSpec;

static const int MAX_BINARY_DATA_LENGTH=10000000;

SubmitMatch_Data::SubmitMatch_Data()
{
	matchBinaryData=0;
}	
SubmitMatch_Data::~SubmitMatch_Data()
{
	if (matchBinaryData)
		delete [] matchBinaryData;
}
void SubmitMatch_Data::Serialize(RakNet::BitStream *bs)
{
	bs->WriteAlignedBytes((const unsigned char*)&participantADbId,sizeof(participantADbId));
	bs->WriteAlignedBytes((const unsigned char*)&participantBDbId,sizeof(participantBDbId));
	bs->Write(participantAScore);
	bs->Write(participantBScore);
	bs->Write(participantAOldRating);
	bs->Write(participantANewRating);
	bs->Write(participantBOldRating);
	bs->Write(participantBNewRating);
	bs->WriteAlignedBytes((const unsigned char*)  &gameDbId,sizeof(gameDbId));
	stringCompressor->EncodeString(matchNotes.C_String(), 4096, bs, 0);
	bs->Write(matchTime);
	bs->WriteAlignedBytesSafe((const char*) matchBinaryData,matchBinaryDataLength,MAX_BINARY_DATA_LENGTH);
}
bool SubmitMatch_Data::Deserialize(RakNet::BitStream *bs)
{
	bool b;

	bs->ReadAlignedBytes((unsigned char*)&participantADbId,sizeof(participantADbId));
	bs->ReadAlignedBytes((unsigned char*)&participantBDbId,sizeof(participantBDbId));
	bs->Read(participantAScore);
	bs->Read(participantBScore);
	bs->Read(participantAOldRating);
	bs->Read(participantANewRating);
	bs->Read(participantBOldRating);
	bs->Read(participantBNewRating);
	bs->ReadAlignedBytes((unsigned char*)&gameDbId,sizeof(gameDbId));
	stringCompressor->DecodeString(&matchNotes, 4096, bs, 0);
	b=bs->Read(matchTime);
	b=bs->ReadAlignedBytesSafeAlloc(&matchBinaryData, matchBinaryDataLength, MAX_BINARY_DATA_LENGTH);
	return b;
}
void ModifyTrustedIPList_Data::Serialize(RakNet::BitStream *bs)
{
	stringCompressor->EncodeString(ip.C_String(), 4096, bs, 0);
	bs->WriteAlignedBytes((const unsigned char*)&gameDbId,sizeof(gameDbId));
	bs->Write(addToList);
}
bool ModifyTrustedIPList_Data::Deserialize(RakNet::BitStream *bs)
{
	stringCompressor->DecodeString(&ip, 4096, bs, 0);
	bs->ReadAlignedBytes((unsigned char*)&gameDbId,sizeof(gameDbId));
	return bs->Read(addToList);
}
GetTrustedIPList_Data::GetTrustedIPList_Data()
{

}
GetTrustedIPList_Data::~GetTrustedIPList_Data()
{
	unsigned i;
	for (i=0; i < ipList.Size(); i++)
	{
		ipList[i]->Deref();
	}
}
GetHistoryForParticipant_Data::~GetHistoryForParticipant_Data()
{
	unsigned i;
	for (i=0; i < matchHistoryList.Size(); i++)
	{
		matchHistoryList[i]->Deref();
	}
}
void GetRatingForParticipant_Data::Serialize(RakNet::BitStream *bs)
{
	bs->WriteAlignedBytes((const unsigned char*)&participantDbId,sizeof(participantDbId));
	bs->WriteAlignedBytes((const unsigned char*)&gameDbId,sizeof(gameDbId));
	bs->Write(participantFound);
	bs->Write(rating);
}
bool GetRatingForParticipant_Data::Deserialize(RakNet::BitStream *bs)
{
	bs->ReadAlignedBytes((unsigned char*)&participantDbId,sizeof(participantDbId));
	bs->ReadAlignedBytes((unsigned char*)&gameDbId,sizeof(gameDbId));
	bs->Read(participantFound);
	return bs->Read(rating);
}