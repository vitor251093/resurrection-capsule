#include "LobbyDBSpec.h"
#include "BitStream.h"
#include "StringCompressor.h"
#include "RakAssert.h"
#include "StringCompressor.h"

using namespace LobbyDBSpec;

static const int MAX_BINARY_DATA_LENGTH=10000000;

CreateUser_Data::CreateUser_Data() {binaryData=0; binaryDataLength=0;}
CreateUser_Data::~CreateUser_Data()
{
	if (binaryData)
		delete [] binaryData;
}
void CreateUser_Data::Serialize(RakNet::BitStream *bs)
{
	stringCompressor->EncodeString(handle.C_String(), 4096, bs, 0);
	SerializeCCInfo(bs);
	SerializeBinaryData(bs);
	SerializePersonalInfo(bs);
	SerializeEmailAddr(bs);
	SerializePassword(bs);
	SerializeCaptions(bs);
	SerializeOtherInfo(bs);
	SerializePermissions(bs);
	SerializeAccountNumber(bs);
	SerializeAdminLevel(bs);
	SerializeAccountBalance(bs);
}
bool CreateUser_Data::Deserialize(RakNet::BitStream *bs)
{
	stringCompressor->DecodeString(&handle, 4096, bs, 0);
	DeserializeCCInfo(bs);
	DeserializeBinaryData(bs);
	DeserializePersonalInfo(bs);
	DeserializeEmailAddr(bs);
	DeserializePassword(bs);
	DeserializeCaptions(bs);
	DeserializeOtherInfo(bs);
	DeserializePermissions(bs);
	DeserializeAccountNumber(bs);
	DeserializeAdminLevel(bs);
	return DeserializeAccountBalance(bs);

}
void CreateUser_Data::SerializeCCInfo(RakNet::BitStream *bs)
{
	stringCompressor->EncodeString(billingAddress1.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(billingAddress2.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(billingCity.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(billingState.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(billingProvince.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(billingCountry.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(billingZipCode.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(creditCardNumber.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(creditCardExpiration.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(creditCardCVV.C_String(), 4096, bs, 0);
}
void CreateUser_Data::SerializeBinaryData(RakNet::BitStream *bs)
{
	bs->WriteAlignedBytesSafe((const char*) binaryData,binaryDataLength,MAX_BINARY_DATA_LENGTH);
}
void CreateUser_Data::SerializePersonalInfo(RakNet::BitStream *bs)
{
	stringCompressor->EncodeString(firstName.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(middleName.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(lastName.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(race.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(sex.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(homeAddress1.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(homeAddress2.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(homeCity.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(homeState.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(homeProvince.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(homeCountry.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(homeZipCode.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(dateOfBirth.C_String(), 4096, bs, 0);
}
void CreateUser_Data::SerializeEmailAddr(RakNet::BitStream *bs)
{
	stringCompressor->EncodeString(emailAddress.C_String(), 4096, bs, 0);
}
void CreateUser_Data::SerializePassword(RakNet::BitStream *bs)
{
	stringCompressor->EncodeString(password.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(passwordRecoveryQuestion.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(passwordRecoveryAnswer.C_String(), 4096, bs, 0);
}
void CreateUser_Data::SerializeCaptions(RakNet::BitStream *bs)
{
	stringCompressor->EncodeString(caption1.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(caption2.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(caption3.C_String(), 4096, bs, 0);
}
void CreateUser_Data::SerializeOtherInfo(RakNet::BitStream *bs)
{
	stringCompressor->EncodeString(sourceIPAddress.C_String(), 4096, bs, 0);
}
void CreateUser_Data::SerializeAccountNumber(RakNet::BitStream *bs)
{
	bs->Write(accountNumber);
}
void CreateUser_Data::SerializeAdminLevel(RakNet::BitStream *bs)
{
	stringCompressor->EncodeString(adminLevel.C_String(), 4096, bs, 0);
}
void CreateUser_Data::SerializeAccountBalance(RakNet::BitStream *bs)
{
	bs->Write(accountBalance);
}
void CreateUser_Data::SerializePermissions(RakNet::BitStream *bs)
{
	stringCompressor->EncodeString(permissions.C_String(), 4096, bs, 0);
}
bool CreateUser_Data::DeserializeCCInfo(RakNet::BitStream *bs)
{
	stringCompressor->DecodeString(&billingAddress1, 4096, bs, 0);
	stringCompressor->DecodeString(&billingAddress2, 4096, bs, 0);
	stringCompressor->DecodeString(&billingCity, 4096, bs, 0);
	stringCompressor->DecodeString(&billingState, 4096, bs, 0);
	stringCompressor->DecodeString(&billingProvince, 4096, bs, 0);
	stringCompressor->DecodeString(&billingCountry, 4096, bs, 0);
	stringCompressor->DecodeString(&billingZipCode, 4096, bs, 0);
	stringCompressor->DecodeString(&creditCardNumber, 4096, bs, 0);
	stringCompressor->DecodeString(&creditCardExpiration, 4096, bs, 0);
	return stringCompressor->DecodeString(&creditCardCVV, 4096, bs, 0);
}
bool CreateUser_Data::DeserializeBinaryData(RakNet::BitStream *bs)
{
	return bs->ReadAlignedBytesSafeAlloc(&binaryData,binaryDataLength, MAX_BINARY_DATA_LENGTH);
}
bool CreateUser_Data::DeserializePersonalInfo(RakNet::BitStream *bs)
{
	stringCompressor->DecodeString(&firstName, 4096, bs, 0);
	stringCompressor->DecodeString(&middleName, 4096, bs, 0);
	stringCompressor->DecodeString(&lastName, 4096, bs, 0);
	stringCompressor->DecodeString(&race, 4096, bs, 0);
	stringCompressor->DecodeString(&sex, 4096, bs, 0);
	stringCompressor->DecodeString(&homeAddress1, 4096, bs, 0);
	stringCompressor->DecodeString(&homeAddress2, 4096, bs, 0);
	stringCompressor->DecodeString(&homeCity, 4096, bs, 0);
	stringCompressor->DecodeString(&homeState, 4096, bs, 0);
	stringCompressor->DecodeString(&homeProvince, 4096, bs, 0);
	stringCompressor->DecodeString(&homeCountry, 4096, bs, 0);
	stringCompressor->DecodeString(&homeZipCode, 4096, bs, 0);
	return stringCompressor->DecodeString(&dateOfBirth, 4096, bs, 0);
}
bool CreateUser_Data::DeserializeEmailAddr(RakNet::BitStream *bs)
{
	return stringCompressor->DecodeString(&emailAddress, 4096, bs, 0);
}
bool CreateUser_Data::DeserializePassword(RakNet::BitStream *bs)
{
	stringCompressor->DecodeString(&password, 4096, bs, 0);
	stringCompressor->DecodeString(&passwordRecoveryQuestion, 4096, bs, 0);
	return stringCompressor->DecodeString(&passwordRecoveryAnswer, 4096, bs, 0);
}
bool CreateUser_Data::DeserializeCaptions(RakNet::BitStream *bs)
{
	stringCompressor->DecodeString(&caption1, 4096, bs, 0);
	stringCompressor->DecodeString(&caption2, 4096, bs, 0);
	return stringCompressor->DecodeString(&caption3, 4096, bs, 0);
}
bool CreateUser_Data::DeserializeOtherInfo(RakNet::BitStream *bs)
{
	bs->Read(accountNumber);
	return stringCompressor->DecodeString(&sourceIPAddress, 4096, bs, 0);
}
bool CreateUser_Data::DeserializeAccountNumber(RakNet::BitStream *bs)
{
	return bs->Read(accountNumber);
}
bool CreateUser_Data::DeserializeAdminLevel(RakNet::BitStream *bs)
{
	return stringCompressor->DecodeString(&adminLevel, 4096, bs, 0);
}
bool CreateUser_Data::DeserializeAccountBalance(RakNet::BitStream *bs)
{
	return bs->Read(accountBalance);
}
bool CreateUser_Data::DeserializePermissions(RakNet::BitStream *bs)
{
	return stringCompressor->DecodeString(&permissions, 4096, bs, 0);
}
void UpdateUser_Data::Serialize(RakNet::BitStream *bs)
{
	SerializeBooleans(bs);
	if (updateCCInfo)
		input.SerializeCCInfo(bs);
	if (updateBinaryData)
		input.SerializeBinaryData(bs);
	if (updatePersonalInfo)
		input.SerializePersonalInfo(bs);
	if (updateEmailAddr)
		input.SerializeEmailAddr(bs);
	if (updatePassword)
		input.SerializePassword(bs);
	if (updateCaptions)
		input.SerializeCaptions(bs);
	if (updateOtherInfo)
		input.SerializeOtherInfo(bs);
	if (updatePermissions)
		input.SerializePermissions(bs);
	if (updateAccountNumber)
		input.SerializeAccountNumber(bs);
	if (updateAdminLevel)
		input.SerializeAdminLevel(bs);
	if (updateAccountBalance)
		input.SerializeAccountBalance(bs);
}
bool UpdateUser_Data::Deserialize(RakNet::BitStream *bs)
{
	return Deserialize(bs, &input, true);
}
bool UpdateUser_Data::Deserialize(RakNet::BitStream *bs, CreateUser_Data *output, bool deserializeBooleans)
{
	bool ret;
	if (deserializeBooleans)
		ret = DeserializeBooleans(bs);
	else
		ret=true;
	if (updateCCInfo)
		ret=output->DeserializeCCInfo(bs);
	if (updateBinaryData)
		ret=output->DeserializeBinaryData(bs);
	if (updatePersonalInfo)
		ret=output->DeserializePersonalInfo(bs);
	if (updateEmailAddr)
		ret=output->DeserializeEmailAddr(bs);
	if (updatePassword)
		ret=output->DeserializePassword(bs);
	if (updateCaptions)
		ret=output->DeserializeCaptions(bs);
	if (updateOtherInfo)
		ret=output->DeserializeOtherInfo(bs);
	if (updatePermissions)
		ret=output->DeserializePermissions(bs);
	if (updateAccountNumber)
		ret=output->DeserializeAccountNumber(bs);
	if (updateAdminLevel)
		ret=output->DeserializeAdminLevel(bs);
	if (updateAccountBalance)
		ret=output->DeserializeAccountBalance(bs);
	return ret;
}
void UpdateUser_Data::SerializeBooleans(RakNet::BitStream *bs)
{
	bs->Write(updateCCInfo);
	bs->Write(updateBinaryData);
	bs->Write(updatePersonalInfo);
	bs->Write(updateEmailAddr);
	bs->Write(updatePassword);
	bs->Write(updateCaptions);
	bs->Write(updateOtherInfo);
	bs->Write(updatePermissions);
	bs->Write(updateAccountNumber);
	bs->Write(updateAdminLevel);
	bs->Write(updateAccountBalance);
}
bool UpdateUser_Data::DeserializeBooleans(RakNet::BitStream *bs)
{
	bs->Read(updateCCInfo);
	bs->Read(updateBinaryData);
	bs->Read(updatePersonalInfo);
	bs->Read(updateEmailAddr);
	bs->Read(updatePassword);
	bs->Read(updateCaptions);
	bs->Read(updateOtherInfo);
	bs->Read(updatePermissions);
	bs->Read(updateAccountNumber);
	bs->Read(updateAdminLevel);
	return bs->Read(updateAccountBalance);
}
SendEmail_Data::SendEmail_Data() 
{
	attachment=0; attachmentLength=0;
}
SendEmail_Data::~SendEmail_Data()
{
	if (attachment)
		delete [] attachment;
}
void SendEmail_Data::Serialize(RakNet::BitStream *bs)
{
	bs->Write(id.hasDatabaseRowId);
	bs->Write(id.databaseRowId);
	stringCompressor->EncodeString(id.handle.C_String(), 512, bs, 0);
	bs->Write(to.Size());
	unsigned i;
	for (i=0; i < to.Size(); i++)
	{
		bs->Write(to[i].hasDatabaseRowId);
		if (to[i].hasDatabaseRowId)
			bs->Write(to[i].databaseRowId);
		else
			stringCompressor->EncodeString(to[i].handle.C_String(), 512, bs, 0);
	}
	stringCompressor->EncodeString(subject.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(body.C_String(), 4096, bs, 0);
	bs->WriteAlignedBytesSafe((const char*) attachment, attachmentLength,MAX_BINARY_DATA_LENGTH);
	bs->Write(initialSenderStatus);
	bs->Write(initialRecipientStatus);
	bs->Write(status);
	bs->Write(creationTime);
	bs->Write(emailMessageID);
	bs->Write(wasOpened);
}
bool SendEmail_Data::Deserialize(RakNet::BitStream *bs)
{
	bs->Read(id.hasDatabaseRowId);
	bs->Read(id.databaseRowId);
	stringCompressor->DecodeString(&id.handle, 512, bs, 0);
	to.Clear(true);
	unsigned toSize;
	bs->Read(toSize);
	unsigned i;
	RowIdOrHandle uioh;
	for (i=0; i < toSize; i++)
	{
		bs->Read(uioh.hasDatabaseRowId);
		if (uioh.hasDatabaseRowId)
			bs->Read(uioh.databaseRowId);
		else
			stringCompressor->DecodeString(&uioh.handle, 512, bs, 0);
		to.Insert(uioh);
	}
	stringCompressor->DecodeString(&subject, 4096, bs, 0);
	stringCompressor->DecodeString(&body, 4096, bs, 0);
	bs->ReadAlignedBytesSafeAlloc(&attachment, attachmentLength,MAX_BINARY_DATA_LENGTH);
	bs->Read(initialSenderStatus);
	bs->Read(initialRecipientStatus);
	bs->Read(status);
	bs->Read(creationTime);
	bs->Read(emailMessageID);
	return bs->Read(wasOpened);
}
GetAccountNotes_Data::~GetAccountNotes_Data()
{
	for (unsigned i=0; i < accountNotes.Size(); i++)
		accountNotes[i]->Deref();
}
GetFriends_Data::~GetFriends_Data()
{
	for (unsigned i=0; i < friends.Size(); i++)
		friends[i]->Deref();
}
GetIgnoreList_Data::~GetIgnoreList_Data()
{
	for (unsigned i=0; i < ignoredUsers.Size(); i++)
		ignoredUsers[i]->Deref();
}
GetEmails_Data::~GetEmails_Data()
{
	for (unsigned i=0; i < emails.Size(); i++)
		emails[i]->Deref();
}
GetActionHistory_Data::~GetActionHistory_Data()
{
	for (unsigned i=0; i < history.Size(); i++)
		history[i]->Deref();
}
void GetActionHistory_Data::Serialize(RakNet::BitStream *bs)
{
	bs->Write(ascendingSort);
	bs->WriteCompressed(history.Size());
	unsigned i;
	for (i=0; i < history.Size(); i++)
		history[i]->Serialize(bs);
}
bool GetActionHistory_Data::Deserialize(RakNet::BitStream *bs)
{
	bool b;
	unsigned historySize;
	bs->Read(ascendingSort);
	b = bs->ReadCompressed(historySize);
	unsigned i;
	AddToActionHistory_Data *data;
	for (i=0; i < historySize; i++)
	{
		data = new AddToActionHistory_Data;
		b=data->Deserialize(bs);
		history.Insert(data);
	}
	return b;
}
void AddToActionHistory_Data::Serialize(RakNet::BitStream *bs)
{
	bs->Write(id.hasDatabaseRowId);
	bs->Write(id.databaseRowId);
	stringCompressor->EncodeString(id.handle.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(actionTime.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(actionTaken.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(description.C_String(), 4096, bs, 0);
	stringCompressor->EncodeString(sourceIpAddress.C_String(), 4096, bs, 0);
	bs->Write(creationTime);
}
bool AddToActionHistory_Data::Deserialize(RakNet::BitStream *bs)
{
	bs->Read(id.hasDatabaseRowId);
	bs->Read(id.databaseRowId);
	stringCompressor->DecodeString(&id.handle, 4096, bs, 0);
	stringCompressor->DecodeString(&actionTime, 4096, bs, 0);
	stringCompressor->DecodeString(&actionTaken, 4096, bs, 0);
	stringCompressor->DecodeString(&description, 4096, bs, 0);
	stringCompressor->DecodeString(&sourceIpAddress, 4096, bs, 0);
	return bs->Read(creationTime);
}
UpdateClanMember_Data::UpdateClanMember_Data()
{
	binaryData=0;
	binaryDataLength=0;
	SetUpdateInts(false);
	SetUpdateFloats(false);
	SetUpdateDescriptions(false);
	updateMEStatus1=false;
	hasRequiredLastStatus=false;
	mEStatus1=CLAN_MEMBER_STATUS_LEADER;

}
UpdateClanMember_Data::~UpdateClanMember_Data()
{
	if (binaryData)
		delete [] binaryData;
}
void UpdateClanMember_Data::Serialize(RakNet::BitStream *bs)
{
	bs->Write(clanId.hasDatabaseRowId);
	bs->Write(clanId.databaseRowId);
	stringCompressor->EncodeString(&clanId.handle, 512, bs, 0);
	bs->Write(userId.hasDatabaseRowId);
	bs->Write(userId.databaseRowId);
	stringCompressor->EncodeString(&userId.handle, 512, bs, 0);
	bs->Write(mEStatus1);
	bs->Write(lastUpdate);
	unsigned i;
	for (i=0; i < sizeof(integers) / sizeof(integers[0]); i++)
		bs->Write(integers[i]);
	for (i=0; i < sizeof(floats) / sizeof(floats[0]); i++)
		bs->Write(floats[i]);
	stringCompressor->EncodeString(&descriptions[0], 4096, bs, 0);
	stringCompressor->EncodeString(&descriptions[1], 4096, bs, 0);
	bs->WriteAlignedBytesSafe((const char*) binaryData,binaryDataLength,MAX_BINARY_DATA_LENGTH);
}
bool UpdateClanMember_Data::Deserialize(RakNet::BitStream *bs)
{
	bs->Read(clanId.hasDatabaseRowId);
	bs->Read(clanId.databaseRowId);
	stringCompressor->DecodeString(&clanId.handle, 512, bs, 0);
	bs->Read(userId.hasDatabaseRowId);
	bs->Read(userId.databaseRowId);
	stringCompressor->DecodeString(&userId.handle, 512, bs, 0);
	bs->Read(mEStatus1);
	bs->Read(lastUpdate);
	unsigned i;
	for (i=0; i < sizeof(integers) / sizeof(integers[0]); i++)
		bs->Read(integers[i]);
	for (i=0; i < sizeof(floats) / sizeof(floats[0]); i++)
		bs->Read(floats[i]);
	stringCompressor->DecodeString(&descriptions[0], 4096, bs, 0);
	stringCompressor->DecodeString(&descriptions[1], 4096, bs, 0);
	return bs->ReadAlignedBytesSafeAlloc(&binaryData,binaryDataLength,MAX_BINARY_DATA_LENGTH);
}
UpdateClan_Data::UpdateClan_Data()
{
	binaryData=0;
	binaryDataLength=0;
	SetUpdateInts(false);
	SetUpdateFloats(false);
	SetUpdateDescriptions(false);
	updateRequiresInvitationsToJoin=false;
	requiresInvitationsToJoin=true;
}
UpdateClan_Data::~UpdateClan_Data()
{
	if (binaryData)
		delete [] binaryData;

	unsigned i;
	for (i=0; i < members.Size(); i++)
		delete members[i];
}
void UpdateClan_Data::Serialize(RakNet::BitStream *bs)
{
	bs->Write(clanId.hasDatabaseRowId);
	bs->Write(clanId.databaseRowId);
	stringCompressor->EncodeString(&clanId.handle, 512, bs, 0);
	stringCompressor->EncodeString(handle, 512, bs, 0);
	unsigned i;
	for (i=0; i < sizeof(updateInts) / sizeof(updateInts[0]); i++)
	{
		bs->Write(updateInts[i]);
		bs->Write(integers[i]);
	}
	for (i=0; i < sizeof(updateFloats) / sizeof(updateFloats[0]); i++)
	{
		bs->Write(updateFloats[i]);
		bs->Write(floats[i]);
	}
	for (i=0; i < 2; i++)
	{
		bs->Write(updateDescriptions[i]);
		stringCompressor->EncodeString(&descriptions[i], 4096, bs, 0);
	}
	bs->Write(updateBinaryData);
	if (updateBinaryData)
		bs->WriteAlignedBytesSafe((const char*) binaryData,binaryDataLength,MAX_BINARY_DATA_LENGTH);
	bs->Write(updateRequiresInvitationsToJoin);
	bs->Write(requiresInvitationsToJoin);
	bs->Write(creationTime);
	bs->Write(members.Size());
	for (i=0; i < members.Size(); i++)
		members[i]->Serialize(bs);
}
bool UpdateClan_Data::Deserialize(RakNet::BitStream *bs)
{
	bs->Read(clanId.hasDatabaseRowId);
	bs->Read(clanId.databaseRowId);
	stringCompressor->DecodeString(&clanId.handle, 512, bs, 0);
	stringCompressor->DecodeString(&handle, 512, bs, 0);
	unsigned i;
	for (i=0; i < sizeof(updateInts) / sizeof(updateInts[0]); i++)
	{
		bs->Read(updateInts[i]);
		bs->Read(integers[i]);
	}
	for (i=0; i < sizeof(updateFloats) / sizeof(updateFloats[0]); i++)
	{
		bs->Read(updateFloats[i]);
		bs->Read(floats[i]);
	}
	for (i=0; i < 2; i++)
	{
		bs->Read(updateDescriptions[i]);
		stringCompressor->DecodeString(&descriptions[i], 4096, bs, 0);
	}
	bs->Read(updateBinaryData);
	if (updateBinaryData)
		bs->ReadAlignedBytesSafeAlloc(&binaryData,binaryDataLength,MAX_BINARY_DATA_LENGTH);
	bs->Read(updateRequiresInvitationsToJoin);
	bs->Read(requiresInvitationsToJoin);
	bs->Read(creationTime);
	unsigned int memberSize;
	UpdateClanMember_Data *clanMember;
	for (i=0; i < members.Size(); i++)
		delete members[i];
	members.Clear();
	bool b;
	b=bs->Read(memberSize);
	for (i=0; i < memberSize; i++)
	{
		clanMember = new UpdateClanMember_Data;
		b=clanMember->Deserialize(bs);
		members.Insert(clanMember);
	}
	return b;
}
void CreateClan_Data::Serialize(RakNet::BitStream *bs)
{
	initialClanData.Serialize(bs);
	leaderData.Serialize(bs);
}
bool CreateClan_Data::Deserialize(RakNet::BitStream *bs)
{
	initialClanData.Deserialize(bs);
	return leaderData.Deserialize(bs);
}
void ChangeClanHandle_Data::Serialize(RakNet::BitStream *bs)
{
	bs->Write(clanId.hasDatabaseRowId);
	bs->Write(clanId.databaseRowId);
	stringCompressor->EncodeString(&clanId.handle, 512, bs, 0);
	stringCompressor->EncodeString(&newHandle, 512, bs, 0);
	stringCompressor->EncodeString(&failureMessage, 512, bs, 0);
}
bool ChangeClanHandle_Data::Deserialize(RakNet::BitStream *bs)
{
	bs->Read(clanId.hasDatabaseRowId);
	bs->Read(clanId.databaseRowId);
	stringCompressor->DecodeString(&clanId.handle, 512, bs, 0);
	stringCompressor->DecodeString(&newHandle, 512, bs, 0);
	return stringCompressor->DecodeString(&failureMessage, 512, bs, 0);
}
GetClans_Data::GetClans_Data()
{
}
GetClans_Data::~GetClans_Data()
{
	unsigned i;
	for (i=0; i < clans.Size(); i++)
		delete clans[i];
}
RemoveFromClan_Data::RemoveFromClan_Data()
{
	hasRequiredLastStatus=false;
}
RemoveFromClan_Data::~RemoveFromClan_Data()
{

}
void GetClans_Data::Serialize(RakNet::BitStream *bs)
{
	bs->Write(ascendingSort);
	bs->Write(userId.databaseRowId);
	stringCompressor->EncodeString(userId.handle.C_String(), 512, bs, 0);
	bs->Write(clans.Size());
	for (unsigned i=0; i < clans.Size(); i++)
		clans[i]->Serialize(bs);


	// [out] All clans for this user
	DataStructures::List<UpdateClan_Data*> clans;
}
bool GetClans_Data::Deserialize(RakNet::BitStream *bs)
{
	bs->Read(ascendingSort);
	bs->Read(userId.databaseRowId);
	stringCompressor->DecodeString(&userId.handle, 512, bs, 0);
	UpdateClan_Data* clanData;
	unsigned int clansSize;
	bool b;
	b=bs->Read(clansSize);
	for (unsigned i=0; i < clansSize; i++)
	{
		clanData = new UpdateClan_Data;
		b=clanData->Deserialize(bs);
		clans.Insert(clanData);
	}
	return b;
}
AddToClanBoard_Data::AddToClanBoard_Data()
{
	binaryData=0;
	binaryDataLength=0;
}
AddToClanBoard_Data::~AddToClanBoard_Data()
{
	if (binaryData)
		delete [] binaryData;
}
void AddToClanBoard_Data::Serialize(RakNet::BitStream *bs)
{
	bs->Write(clanId.hasDatabaseRowId);
	bs->Write(clanId.databaseRowId);
	stringCompressor->EncodeString(&clanId.handle, 512, bs, 0);

	bs->Write(userId.hasDatabaseRowId);
	bs->Write(userId.databaseRowId);
	stringCompressor->EncodeString(&userId.handle, 512, bs, 0);

	bs->Write(postId);
	stringCompressor->EncodeString(&subject, 4096, bs, 0);
	stringCompressor->EncodeString(&body, 4096, bs, 0);
	bs->Write(creationTime);

	unsigned i;
	for (i=0; i < sizeof(integers) / sizeof(integers[0]); i++)
		bs->Write(integers[i]);
	for (i=0; i < sizeof(floats) / sizeof(floats[0]); i++)
		bs->Write(floats[i]);
	bs->WriteAlignedBytesSafe((const char*) binaryData,binaryDataLength,MAX_BINARY_DATA_LENGTH);
}
bool AddToClanBoard_Data::Deserialize(RakNet::BitStream *bs)
{
	bs->Read(clanId.hasDatabaseRowId);
	bs->Read(clanId.databaseRowId);
	stringCompressor->DecodeString(&clanId.handle, 512, bs, 0);

	bs->Read(userId.hasDatabaseRowId);
	bs->Read(userId.databaseRowId);
	stringCompressor->DecodeString(&userId.handle, 512, bs, 0);

	bs->Read(postId);
	stringCompressor->DecodeString(&subject, 4096, bs, 0);
	stringCompressor->DecodeString(&body, 4096, bs, 0);
	bs->Read(creationTime);

	unsigned i;
	for (i=0; i < sizeof(integers) / sizeof(integers[0]); i++)
		bs->Read(integers[i]);
	for (i=0; i < sizeof(floats) / sizeof(floats[0]); i++)
		bs->Read(floats[i]);
	return bs->ReadAlignedBytesSafeAlloc(&binaryData,binaryDataLength,MAX_BINARY_DATA_LENGTH);
}
GetClanBoard_Data::GetClanBoard_Data()
{
}
GetClanBoard_Data::~GetClanBoard_Data()
{
	unsigned i;
	for (i=0; i < board.Size(); i++)
		delete board[i];
}
