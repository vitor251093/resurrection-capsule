/// \file
/// \brief An implementation of the AutopatcherRepositoryInterface to use PostgreSQL to store the relevant data
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "AutopatcherPostgreRepository.h"
#include "AutopatcherPatchContext.h"
#include "FileList.h"
// libpq-fe.h is part of PostgreSQL which must be installed on this computer to use the PostgreRepository
#include "libpq-fe.h"
#include "CreatePatch.h"
#include "AutopatcherPatchContext.h"
// #include "SHA1.h"
#include <stdlib.h>
#include "LinuxStrings.h"

static const unsigned HASH_LENGTH=sizeof(unsigned int);

// ntohl
#ifdef _WIN32
#include <Winsock2.h>
#else
#include <netinet/in.h>
#endif

// alloca
#ifdef _COMPATIBILITY_1
#elif defined(_WIN32)
#include <malloc.h>
#else
//#include <stdlib.h>
#endif

#define PQEXECPARAM_FORMAT_TEXT		0
#define PQEXECPARAM_FORMAT_BINARY	1

AutopatcherPostgreRepository::AutopatcherPostgreRepository()
{
}
AutopatcherPostgreRepository::~AutopatcherPostgreRepository()
{
}
bool AutopatcherPostgreRepository::CreateAutopatcherTables(void)
{
	if (isConnected==false)
		return false;

	const char *command =
		"BEGIN;"
	"CREATE TABLE Applications ("
		"applicationID serial PRIMARY KEY UNIQUE,"
		"applicationName text NOT NULL UNIQUE,"
		"changeSetID integer NOT NULL DEFAULT 0,"
		"userName text NOT NULL"
		");"
	"CREATE TABLE FileVersionHistory ("
		"fileID serial PRIMARY KEY UNIQUE,"
		"applicationID integer REFERENCES Applications ON DELETE CASCADE,"
		"filename text NOT NULL,"
		"fileLength integer,"
		"content bytea,"
		"contentHash bytea,"
		"patch bytea,"
		"createFile boolean NOT NULL,"
		"modificationDate timestamp NOT NULL DEFAULT LOCALTIMESTAMP,"
		"lastSentDate timestamp,"
		"timesSent integer NOT NULL DEFAULT 0,"
		"changeSetID integer NOT NULL,"
		"userName text NOT NULL,"
		"CONSTRAINT file_has_data CHECK ( createFile=FALSE OR ((content IS NOT NULL) AND (contentHash IS NOT NULL) AND (fileLength IS NOT NULL) ) )"
		");"
	"CREATE VIEW AutoPatcherView AS SELECT "
		"FileVersionHistory.applicationid,"
		"Applications.applicationName,"
		"FileVersionHistory.fileID,"
		"FileVersionHistory.fileName,"
		"FileVersionHistory.createFile,"
		"FileVersionHistory.fileLength,"
		"FileVersionHistory.changeSetID,"
		"FileVersionHistory.lastSentDate," 
		"FileVersionHistory.modificationDate,"
		"FileVersionHistory.timesSent "
	"FROM (FileVersionHistory JOIN Applications ON "
		"( FileVersionHistory.applicationID = Applications.applicationID )) "
	"ORDER BY Applications.applicationID ASC, FileVersionHistory.fileID ASC;"
	"COMMIT;";

	PGresult *result;
	//sqlCommandMutex.Lock();
	bool res = ExecuteBlockingCommand(command, &result, true);
	//sqlCommandMutex.Unlock();
	PQclear(result);
	return res;
}

bool AutopatcherPostgreRepository::DestroyAutopatcherTables(void)
{
	if (isConnected==false)
		return false;

	const char *command = 
		"BEGIN;"
		"DROP TABLE Applications CASCADE;"
		"DROP TABLE FileVersionHistory CASCADE;"
		"COMMIT;";

	PGresult *result;
	//sqlCommandMutex.Lock();
	bool b = ExecuteBlockingCommand(command, &result, true);
	//sqlCommandMutex.Unlock();
	PQclear(result);
	return b;
}
bool AutopatcherPostgreRepository::AddApplication(const char *applicationName, const char *userName)
{
	char query[512];
	if (strlen(applicationName)>100)
		return false;
	if (strlen(userName)>100)
		return false;

	sprintf(query, "INSERT INTO Applications (applicationName, userName) VALUES ('%s', '%s');", GetEscapedString(applicationName).C_String(), GetEscapedString(userName).C_String());
	PGresult *result;
	//sqlCommandMutex.Lock();
	bool b = ExecuteBlockingCommand(query, &result, false);
	//sqlCommandMutex.Unlock();
	PQclear(result);
	return b;
}
bool AutopatcherPostgreRepository::RemoveApplication(const char *applicationName)
{
	char query[512];
	if (strlen(applicationName)>100)
		return false;
	
	sprintf(query, "DELETE FROM Applications WHERE applicationName='%s';", GetEscapedString(applicationName).C_String());
	PGresult *result;
	//sqlCommandMutex.Lock();
	bool b = ExecuteBlockingCommand(query, &result, false);
	//sqlCommandMutex.Unlock();
	PQclear(result);
	return b;
}

bool AutopatcherPostgreRepository::GetChangelistSinceDate(const char *applicationName, FileList *addedFiles, FileList *deletedFiles, const char *sinceDate, char currentDate[64])
{
	PGresult *result;
	char query[512];
	if (strlen(applicationName)>100)
		return false;
	if (strlen(sinceDate)>63)
		return false;
	RakNet::RakString escapedApplicationName = GetEscapedString(applicationName);
	RakNet::RakString escapedSinceDate = GetEscapedString(sinceDate);
	sprintf(query, "SELECT applicationID FROM applications WHERE applicationName='%s';", escapedApplicationName.C_String());
	//sqlCommandMutex.Lock();
	if (ExecuteBlockingCommand(query, &result, false)==false)
	{
		//sqlCommandMutex.Unlock();
		PQclear(result);
		return false;
	}
	//sqlCommandMutex.Unlock();
	int numRows;
	numRows = PQntuples(result);
	if (numRows==0)
	{
		sprintf(lastError,"ERROR: %s not found in UpdateApplicationFiles\n",applicationName);
		return false;
	}
	char *res;
	res = PQgetvalue(result,0,0);
	int applicationID;
	applicationID=atoi(res);
	PQclear(result);
	if (sinceDate && sinceDate[0])
		sprintf(query, "SELECT DISTINCT ON (filename) filename, fileLength, contentHash, createFile FROM FileVersionHistory WHERE applicationId=%i AND modificationDate > '%s' ORDER BY filename, fileId DESC;", applicationID, escapedSinceDate.C_String());
	else
		sprintf(query, "SELECT DISTINCT ON (filename) filename, fileLength, contentHash, createFile FROM FileVersionHistory WHERE applicationId=%i ORDER BY filename, fileId DESC;", applicationID);

	//sqlCommandMutex.Lock();
	result = PQexecParams(pgConn, query,0,0,0,0,0,PQEXECPARAM_FORMAT_BINARY);
	//sqlCommandMutex.Unlock();
	if (IsResultSuccessful(result, false)==false)
	{
		PQclear(result);
		return false;
	}

	int filenameColumnIndex = PQfnumber(result, "filename");
	int contentHashColumnIndex = PQfnumber(result, "contentHash");
	int createFileColumnIndex = PQfnumber(result, "createFile");
	int fileLengthColumnIndex = PQfnumber(result, "fileLength");
	char *hardDriveFilename;
	char *hardDriveHash;
	char *createFileResult;
	char *fileLengthPtr;
	int rowIndex;
	int fileLength;
	RakAssert(PQfsize(result, fileLengthColumnIndex)==sizeof(fileLength));
	
	numRows = PQntuples(result);

	for (rowIndex=0; rowIndex < numRows; rowIndex++)
	{
		createFileResult = PQgetvalue(result, rowIndex, createFileColumnIndex);
		hardDriveFilename = PQgetvalue(result, rowIndex, filenameColumnIndex);
		if (createFileResult[0]==1)
		{
			hardDriveHash = PQgetvalue(result, rowIndex, contentHashColumnIndex);
			fileLengthPtr = PQgetvalue(result, rowIndex, fileLengthColumnIndex);
			memcpy(&fileLength, fileLengthPtr, sizeof(fileLength));
			fileLength=ntohl(fileLength); // This is asinine...
			addedFiles->AddFile(hardDriveFilename, hardDriveFilename, hardDriveHash, HASH_LENGTH, fileLength, FileListNodeContext(0,0), false);
		}
		else
		{
			deletedFiles->AddFile(hardDriveFilename,hardDriveFilename,0,0,0,FileListNodeContext(0,0), false);
		}
	}

	//sqlCommandMutex.Lock();
	if (ExecuteBlockingCommand("SELECT LOCALTIMESTAMP", &result, false)==false)
	{
		//sqlCommandMutex.Unlock();
		PQclear(result);
		return false;
	}
	//sqlCommandMutex.Unlock();

	char *ts=PQgetvalue(result, 0, 0);
	if (ts)
	{
		strncpy(currentDate, ts,63);
		currentDate[63]=0;
	}
	else
	{
		strcpy(lastError, "Can't read current time\n");
		return false;
	}
	
	return true;
}
bool AutopatcherPostgreRepository::GetPatches(const char *applicationName, FileList *input, FileList *patchList, char currentDate[64])
{
	PGresult *result;
	char query[512];
	if (strlen(applicationName)>100)
		return false;
	RakNet::RakString escapedApplicationName = GetEscapedString(applicationName);
	sprintf(query, "SELECT applicationID FROM applications WHERE applicationName='%s';", escapedApplicationName.C_String());
	//sqlCommandMutex.Lock();
	if (ExecuteBlockingCommand(query, &result, false)==false)
	{
		//sqlCommandMutex.Unlock();
		PQclear(result);
		return false;
	}
	//sqlCommandMutex.Unlock();
	int numRows;
	numRows = PQntuples(result);
	if (numRows==0)
	{
		sprintf(lastError,"ERROR: %s not found in UpdateApplicationFiles\n",applicationName);
		return false;
	}
	char *res;
	res = PQgetvalue(result,0,0);
	int applicationID;
	applicationID=atoi(res);
	PQclear(result);

	// Go through the input list.
	unsigned inputIndex;
	char *userHash, *contentHash;
	RakNet::RakString userFilename;
	// char *content;
//	char *fileId, *fileLength;
	char *patch;
	// int contentLength;
	int patchLength;
//	int contentColumnIndex;
	int contentHashIndex, fileIdIndex, fileLengthIndex;
	const char *outTemp[2];
	int outLengths[2];
	int formats[2];
	PGresult *patchResult;
	for (inputIndex=0; inputIndex < input->fileList.Size(); inputIndex++)
	{
		userHash=input->fileList[inputIndex].data;
		userFilename=input->fileList[inputIndex].filename;

		if (userHash==0)
		{
			// If the user does not have a hash in the input list, get the contents of latest version of this named file and write it to the patch list
		//	sprintf(query, "SELECT DISTINCT ON (filename) content FROM FileVersionHistory WHERE applicationId=%i AND filename=$1::text ORDER BY filename, fileId DESC;", applicationID);
			sprintf(query, "SELECT DISTINCT ON (filename) fileId, fileLength FROM FileVersionHistory WHERE applicationId=%i AND filename=$1::text ORDER BY filename, fileId DESC;", applicationID);
			outTemp[0]=userFilename.C_String();
			outLengths[0]=(int) userFilename.GetLength();
			formats[0]=PQEXECPARAM_FORMAT_BINARY;
			//sqlCommandMutex.Lock();
			result = PQexecParams(pgConn, query,1,0,outTemp,outLengths,formats,PQEXECPARAM_FORMAT_BINARY);
			//sqlCommandMutex.Unlock();
			if (IsResultSuccessful(result, false)==false)
			{
				PQclear(result);
				return false;
			}
			numRows = PQntuples(result);
			if (numRows>0)
			{
			//	content = PQgetvalue(result, 0, 0);
			//	contentLength=PQgetlength(result, 0, 0);
			//	patchList->AddFile(userFilename, content, contentLength, contentLength, FileListNodeContext(PC_WRITE_FILE,0),false);

				int fileIdIndex = PQfnumber(result, "fileId");
				int fileLengthIndex = PQfnumber(result, "fileLength");
				int fileId = ntohl(*((int*)PQgetvalue(result, 0, fileIdIndex)));
				int fileLength = ntohl(*((int*)PQgetvalue(result, 0, fileLengthIndex)));


				patchList->AddFile(userFilename,userFilename, 0, fileLength, fileLength, FileListNodeContext(PC_WRITE_FILE,fileId),true);
			}
			PQclear(result);
		}
		else // Assuming the user does have a hash.
		{
			if (input->fileList[inputIndex].dataLengthBytes!=HASH_LENGTH)
				return false;

			// Get the hash and ID of the latest version of this file, by filename.
			sprintf(query, "SELECT DISTINCT ON (filename) contentHash, fileId, fileLength FROM FileVersionHistory WHERE applicationId=%i AND filename=$1::text ORDER BY filename, fileId DESC;", applicationID);
			outTemp[0]=userFilename.C_String();
			outLengths[0]=(int)userFilename.GetLength();
			formats[0]=PQEXECPARAM_FORMAT_BINARY;
			//sqlCommandMutex.Lock();
			result = PQexecParams(pgConn, query,1,0,outTemp,outLengths,formats,PQEXECPARAM_FORMAT_BINARY);
			//sqlCommandMutex.Unlock();
			if (IsResultSuccessful(result, false)==false)
			{
				PQclear(result);
				return false;
			}
			numRows = PQntuples(result);
			if (numRows>0)
			{
				contentHashIndex = PQfnumber(result, "contentHash");
				fileIdIndex = PQfnumber(result, "fileId");
				fileLengthIndex = PQfnumber(result, "fileLength");
				contentHash = PQgetvalue(result, 0, contentHashIndex);
				int fileIdInt = ntohl(*((int*)PQgetvalue(result, 0, fileIdIndex)));
				int fileLengthInt = ntohl(*((int*)PQgetvalue(result, 0, fileLengthIndex)));
//				fileId = PQgetvalue(result, 0, fileIdIndex);
//				fileLength = PQgetvalue(result, 0, fileLengthIndex);
//				int fileLengthInt = ntohl(*((int*)fileLength));

//				unsigned int hash1 = SuperFastHashFile("C:/temp/RakNet_Icon_Final.psd");
//				unsigned int hash2 = SuperFastHashFile("C:/temp/AutopatcherClient/RakNet_Icon_Final.psd");

				if (memcmp(contentHash, userHash, HASH_LENGTH)!=0)
				{
					// Look up by user hash/filename/applicationID, returning the patch
					sprintf(query, "SELECT patch FROM FileVersionHistory WHERE applicationId=%i AND filename=$1::text AND contentHash=$2::bytea;", applicationID);
					outTemp[0]=userFilename.C_String();
					outLengths[0]=(int)userFilename.GetLength();
					formats[0]=PQEXECPARAM_FORMAT_TEXT;
					outTemp[1]=userHash;
					outLengths[1]=HASH_LENGTH;
					formats[1]=PQEXECPARAM_FORMAT_BINARY;
					//sqlCommandMutex.Lock();
					patchResult = PQexecParams(pgConn, query,2,0,outTemp,outLengths,formats,PQEXECPARAM_FORMAT_BINARY);
					//sqlCommandMutex.Unlock();

					if (IsResultSuccessful(patchResult, false)==false)
					{
						PQclear(patchResult);
						return false;
					}
					numRows = PQntuples(patchResult);
					if (numRows==0)
					{
						PQclear(patchResult);

			//			outTemp[0]=fileId;
			//			outLengths[0]=PQfsize(result, fileIdIndex);
			//			formats[0]=PQEXECPARAM_FORMAT_BINARY;

						/*
						// Get 32000000 bytes at a time to workaround http://support.microsoft.com/kb/q201213
						int fileIndex=0;
						char *file = RakNet::OP_NEW char[fileLengthInt];
						while (fileIndex < fileLengthInt)
						{
							sprintf(query, "SELECT substring(content from %i for 32000000) FROM FileVersionHistory WHERE fileId=$1::integer;", fileIndex, fileId);
							patchResult = PQexecParams(pgConn, query,1,0,outTemp,outLengths,formats,PQEXECPARAM_FORMAT_BINARY);
							if (IsResultSuccessful(patchResult, false)==false)
							{
								PQclear(patchResult);
								return false;
							}
							content = PQgetvalue(patchResult, 0, 0);
							contentLength=PQgetlength(patchResult, 0, 0);
							assert(contentLength==32000000 || contentLength==fileLengthInt-fileIndex);
							memcpy(file+fileIndex,content,contentLength);
							PQclear(patchResult);
							fileIndex+=32000000;
						}
*/

						/*
						sprintf(query, "SELECT content FROM FileVersionHistory WHERE fileId=%i", fileIdInt);
						patchResult = PQexecParams(pgConn, query,0,0,outTemp,outLengths,formats,PQEXECPARAM_FORMAT_BINARY);
						if (IsResultSuccessful(patchResult, false)==false)
						{
							PQclear(patchResult);
							return false;
						}
						char *content = PQgetvalue(patchResult, 0, 0);
						int contentLength=PQgetlength(patchResult, 0, 0);
						unsigned int hash1 = SuperFastHashFile("C:/temp/RakNet_Icon_Final.psd");
						unsigned int hash2 = SuperFastHash(content, contentLength);
						*/

						patchList->AddFile(userFilename,userFilename, 0, fileLengthInt, fileLengthInt, FileListNodeContext(PC_WRITE_FILE,fileIdInt), true);
//						patchList->AddFile(userFilename, file, fileLengthInt, contentLength, FileListNodeContext(PC_WRITE_FILE,0), true);
//						RakNet::OP_DELETE_ARRAY file;
					}
					else
					{
						// Otherwise, write the hash of the new version and then write the patch to get to that version.
						// 
						patch = PQgetvalue(patchResult, 0, 0);
						patchLength=PQgetlength(patchResult, 0, 0);
						// Bleh, get a stack overflow here
						// char *temp = (char*) _alloca(patchLength+HASH_LENGTH);
						char *temp = RakNet::OP_NEW_ARRAY<char>(patchLength + HASH_LENGTH, __FILE__, __LINE__ );
						memcpy(temp, contentHash, HASH_LENGTH);
						memcpy(temp+HASH_LENGTH, patch, patchLength);
//						int len;
//						assert(PQfsize(result, fileLengthIndex)==sizeof(fileLength));
//						memcpy(&len, fileLength, sizeof(len));
//						len=ntohl(len);
					//	printf("send patch %i bytes\n", patchLength);
					//	for (int i=0; i < patchLength; i++)
					//		printf("%i ", patch[i]);
					//	printf("\n");
						patchList->AddFile(userFilename,userFilename, temp, HASH_LENGTH+patchLength, fileLengthInt, FileListNodeContext(PC_HASH_WITH_PATCH,0),false );
						PQclear(patchResult);
						RakNet::OP_DELETE_ARRAY(temp, __FILE__, __LINE__);
					}
				}
				else
				{
					// else if the hash of this file matches what the user has, the user has the latest version.  Done.
				}
			}
			else
			{
				// else if there is no such file, skip this file.
			}

			PQclear(result);
		}
	}

	//sqlCommandMutex.Lock();
	if (ExecuteBlockingCommand("SELECT LOCALTIMESTAMP", &result, false)==false)
	{
		//sqlCommandMutex.Unlock();
		PQclear(result);
		return false;
	}
	//sqlCommandMutex.Unlock();

	char *ts=PQgetvalue(result, 0, 0);
	if (ts)
	{
		strncpy(currentDate, ts,63);
		currentDate[63]=0;
	}
	else
	{
		strcpy(lastError, "Can't read current time\n");
		return false;
	}

	return true;
}
bool AutopatcherPostgreRepository::UpdateApplicationFiles(const char *applicationName, const char *applicationDirectory, const char *userName, FileListProgress *cb)
{
	FileList filesOnHarddrive;
	filesOnHarddrive.SetCallback(cb);
	filesOnHarddrive.AddFilesFromDirectory(applicationDirectory,"", true, true, true, FileListNodeContext(0,0));
	if (filesOnHarddrive.fileList.Size()==0)
	{
		sprintf(lastError,"ERROR: Can't find files at %s in UpdateApplicationFiles\n",applicationDirectory);
		return false;
	}

	int numRows;
	PGresult *result;
	char query[512];
	if (strlen(applicationName)>100)
		return false;
	if (strlen(userName)>100)
		return false;

	RakNet::RakString escapedApplicationName = GetEscapedString(applicationName);
	sprintf(query, "SELECT applicationID FROM applications WHERE applicationName='%s';", escapedApplicationName.C_String());
	//sqlCommandMutex.Lock();
	if (ExecuteBlockingCommand(query, &result, false)==false)
	{
		//sqlCommandMutex.Unlock();
		PQclear(result);
		return false;
	}
	//sqlCommandMutex.Unlock();
	numRows = PQntuples(result);
	if (numRows==0)
	{
		sprintf(lastError,"ERROR: %s not found in UpdateApplicationFiles\n",applicationName);
		return false;
	}
	char *res;
	res = PQgetvalue(result,0,0);
	int applicationID;
	applicationID=atoi(res);
	PQclear(result);

	// If ExecuteBlockingCommand fails then it does a rollback
	//sqlCommandMutex.Lock();
	if (ExecuteBlockingCommand("BEGIN;", &result, false)==false)
	{
		//sqlCommandMutex.Unlock();
		PQclear(result);
		return false;
	}
	//sqlCommandMutex.Unlock();
	PQclear(result);

	sprintf(query, "UPDATE applications SET changeSetId = changeSetId + 1 where applicationID=%i; SELECT changeSetId FROM applications WHERE applicationID=%i;", applicationID, applicationID);
	//sqlCommandMutex.Lock();
	if (ExecuteBlockingCommand(query, &result, true)==false)
	{
		//sqlCommandMutex.Unlock();
		PQclear(result);
		return false;
	}
	//sqlCommandMutex.Unlock();
	numRows = PQntuples(result);
	if (numRows==0)
	{
		sprintf(lastError,"ERROR: applicationID %i not found in UpdateApplicationFiles\n", applicationID);
		Rollback();
		return false;
	}
	res = PQgetvalue(result,0,0);
	int changeSetId;
	changeSetId=atoi(res);
	PQclear(result);

	// +1 was added in the update
	changeSetId--;

	// Gets all newest files
	// TODO - This can be non-blocking
	sprintf(query, "SELECT DISTINCT ON (filename) filename, contentHash, createFile FROM FileVersionHistory WHERE applicationID=%i ORDER BY filename, fileId DESC;", applicationID);
	//sqlCommandMutex.Lock();
	result = PQexecParams(pgConn, query,0,0,0,0,0,PQEXECPARAM_FORMAT_BINARY);
	//sqlCommandMutex.Unlock();
	if (IsResultSuccessful(result, true)==false)
	{
		PQclear(result);
		return false;
	}

	int filenameColumnIndex = PQfnumber(result, "filename");
	int contentHashColumnIndex = PQfnumber(result, "contentHash");
	int createFileColumnIndex = PQfnumber(result, "createFile");

	//char *PQgetvalue(result,int row_number,int column_number);
	//int PQgetlength(result, int row_number, int column_number);

	unsigned fileListIndex;
	int rowIndex;
	RakNet::RakString hardDriveFilename;
	char *hardDriveHash;
	RakNet::RakString queryFilename;
	char *createFileResult;
	char *hash;
	bool addFile;
	FileList newFiles;
	numRows = PQntuples(result);
    
	// Loop through files om filesOnHarddrive
	// If the file in filesOnHarddrive does not exist in the query result, or if it does but the hash is different or non-existent, add this file to the create list
	for (fileListIndex=0; fileListIndex < filesOnHarddrive.fileList.Size(); fileListIndex++)
	{
		if (fileListIndex%10==0)
			printf("Hashing files %i/%i\n", fileListIndex+1, filesOnHarddrive.fileList.Size());
		addFile=true;
		hardDriveFilename=filesOnHarddrive.fileList[fileListIndex].filename;
		hardDriveHash=filesOnHarddrive.fileList[fileListIndex].data;
		for (rowIndex=0; rowIndex < numRows; rowIndex++ )
		{
			queryFilename = PQgetvalue(result, rowIndex, filenameColumnIndex);

			if (_stricmp(hardDriveFilename, queryFilename)==0)
			{
				createFileResult = PQgetvalue(result, rowIndex, createFileColumnIndex);
				hash = PQgetvalue(result, rowIndex, contentHashColumnIndex);
				if (createFileResult[0]==1 && memcmp(hash, hardDriveHash, HASH_LENGTH)==0)
				{
					// File exists in database and is the same
					addFile=false;
				}

				break;
			}
		}

		// Unless set to false, file does not exist in query result or is different.
		if (addFile==true)
		{
			newFiles.AddFile(hardDriveFilename,hardDriveFilename, filesOnHarddrive.fileList[fileListIndex].data, filesOnHarddrive.fileList[fileListIndex].dataLengthBytes, filesOnHarddrive.fileList[fileListIndex].fileLengthBytes, FileListNodeContext(0,0), false);
		}
	}
	
	// Go through query results that are marked as create
	// If a file that is currently on the database is not on the harddrive, add it to the delete list
	FileList deletedFiles;
	bool fileOnHarddrive;
	for (rowIndex=0; rowIndex < numRows; rowIndex++ )
	{
		queryFilename = PQgetvalue(result, rowIndex, filenameColumnIndex);
		createFileResult = PQgetvalue(result, rowIndex, createFileColumnIndex);
		if (createFileResult[0]!=1)
			continue; // If already false don't mark false again.

		fileOnHarddrive=false;
		for (fileListIndex=0; fileListIndex < filesOnHarddrive.fileList.Size(); fileListIndex++)
		{
			hardDriveFilename=filesOnHarddrive.fileList[fileListIndex].filename.C_String();
			hardDriveHash=filesOnHarddrive.fileList[fileListIndex].data;

			if (_stricmp(hardDriveFilename, queryFilename)==0)
			{
				fileOnHarddrive=true;
				break;
			}
		}

		if (fileOnHarddrive==false)
			deletedFiles.AddFile(queryFilename,queryFilename,0,0,0,FileListNodeContext(0,0), false);
	}

	// Query memory and files on harddrive no longer needed.  Free this memory since generating all the patches is memory intensive.
	PQclear(result);
	filesOnHarddrive.Clear();

	const char *outTemp[3];
	int outLengths[3];
	int formats[3];
	formats[0]=PQEXECPARAM_FORMAT_TEXT;
	formats[1]=PQEXECPARAM_FORMAT_BINARY; // Always happens to be binary
	formats[2]=PQEXECPARAM_FORMAT_BINARY; // Always happens to be binary

	// For each file in the delete list add a row indicating file deletion
	for (fileListIndex=0; fileListIndex < deletedFiles.fileList.Size(); fileListIndex++)
	{
		if (fileListIndex%10==0)
			printf("Tagging deleted files %i/%i\n", fileListIndex+1, deletedFiles.fileList.Size());

		// BUGGED
		sprintf(query, "INSERT INTO FileVersionHistory(applicationID, filename, createFile, changeSetID, userName) VALUES (%i, $1::text,FALSE,%i,'%s');", applicationID, changeSetId, GetEscapedString(userName).C_String());
		outTemp[0]=deletedFiles.fileList[fileListIndex].filename.C_String();
		outLengths[0]=(int)deletedFiles.fileList[fileListIndex].filename.GetLength();
		formats[0]=PQEXECPARAM_FORMAT_TEXT;
		//sqlCommandMutex.Lock();
		result = PQexecParams(pgConn, query,1,0,outTemp,outLengths,formats,PQEXECPARAM_FORMAT_BINARY);
		//sqlCommandMutex.Unlock();
		if (IsResultSuccessful(result, true)==false)
		{
			deletedFiles.Clear();
			newFiles.Clear();
			PQclear(result);
			return false;
		}
		PQclear(result);
	}
	
	// Clear the delete list as it is no longer needed.
	deletedFiles.Clear();

	int contentColumnIndex;		

	PGresult *uploadResult;
	PGresult *fileRows;
	char *content;
	char *fileID;
	int contentLength;
	int fileIDLength;
	int fileIDColumnIndex;
	char *hardDriveData;
	unsigned hardDriveDataLength;
	char *patch;
	unsigned patchLength;	

	// For each file in the create list
	for (fileListIndex=0; fileListIndex < newFiles.fileList.Size(); fileListIndex++)
	{
		if (fileListIndex%10==0)
			printf("Adding file %i/%i\n", fileListIndex+1, newFiles.fileList.Size());
		hardDriveFilename=newFiles.fileList[fileListIndex].filename;
		hardDriveData=newFiles.fileList[fileListIndex].data+HASH_LENGTH;
		hardDriveHash=newFiles.fileList[fileListIndex].data;
		hardDriveDataLength=newFiles.fileList[fileListIndex].fileLengthBytes;

		sprintf( query, "SELECT fileID from FileVersionHistory WHERE applicationID=%i AND filename=$1::text AND createFile=TRUE;", applicationID );
		outTemp[0]=hardDriveFilename;
		outLengths[0]=(int)strlen(hardDriveFilename);
		formats[0]=PQEXECPARAM_FORMAT_TEXT;
		//sqlCommandMutex.Lock();
		fileRows = PQexecParams(pgConn, query,1,0,outTemp,outLengths,formats,PQEXECPARAM_FORMAT_TEXT);
		//sqlCommandMutex.Unlock();
		if (IsResultSuccessful(fileRows, true)==false)
		{
			newFiles.Clear();
			PQclear(fileRows);
			RakAssert(0);
			return false;
		}
		
		fileIDColumnIndex = PQfnumber( fileRows, "fileID" );
		numRows = PQntuples(fileRows);
		outTemp[0]=hardDriveFilename;
		outLengths[0]=(int)strlen(hardDriveFilename);

		// Create new patches for every create version
		for (rowIndex=0; rowIndex < numRows; rowIndex++ )
		{
			fileIDLength=PQgetlength(fileRows, rowIndex, fileIDColumnIndex);			
			fileID=PQgetvalue(fileRows, rowIndex, fileIDColumnIndex);
			
			formats[0]=PQEXECPARAM_FORMAT_TEXT;
			outTemp[0]=fileID;
			outLengths[0]=fileIDLength;
			
			// The last query handled all the relevant comparisons
			sprintf(query, "SELECT content from FileVersionHistory WHERE fileID=$1::int;" );
			//sqlCommandMutex.Lock();
			result = PQexecParams(pgConn, query,1,0,outTemp,outLengths,formats,PQEXECPARAM_FORMAT_BINARY);
			//sqlCommandMutex.Unlock();
			if (IsResultSuccessful(result, true)==false)
			{
				Rollback();
				newFiles.Clear();
				PQclear(result);
				return false;
			}
			int numContent = PQntuples(result);
			if( numContent > 1 || numContent == 0 )
			{
				Rollback();
				newFiles.Clear();
				PQclear(result);
				PQclear(fileRows);
				RakAssert(0);
				return false;
			}
			formats[0] = PQEXECPARAM_FORMAT_TEXT;

			contentColumnIndex = PQfnumber(result, "content");
			contentLength=PQgetlength(result, 0, contentColumnIndex);
			content=PQgetvalue(result, 0, contentColumnIndex);

			if (CreatePatch(content, contentLength, hardDriveData, hardDriveDataLength, &patch, &patchLength)==false)
			{
				strcpy(lastError,"CreatePatch failed.");
				Rollback();


				newFiles.Clear();
				PQclear(result);
				PQclear(fileRows);
				return false;
			}
			
			outTemp[0]=fileID;
			outLengths[0]=fileIDLength;
			outTemp[1]=patch;
			outLengths[1]=patchLength;
			
			//sqlCommandMutex.Lock();
			uploadResult = PQexecParams(pgConn, "UPDATE FileVersionHistory SET patch=$2::bytea where fileID=$1::int;",2,0,outTemp,outLengths,formats,PQEXECPARAM_FORMAT_TEXT);
			//sqlCommandMutex.Unlock();
			if (IsResultSuccessful(uploadResult, true)==false)
			{
				Rollback();
				newFiles.Clear();
				PQclear(result);
				return false;
			}

			// Done with this patch data
			delete [] patch;

			PQclear(result);
			PQclear(uploadResult);
		}
		PQclear(fileRows);

		sprintf(query, "INSERT INTO FileVersionHistory (applicationID, filename, fileLength, content, contentHash, createFile, changeSetID, userName) "
			"VALUES ("
			"%i,"
			"$1::text,"
			"%i,"
			"$2::bytea,"
			"$3::bytea,"
			"TRUE,"
			"%i,"
			"'%s'"
			");", applicationID, hardDriveDataLength, changeSetId, GetEscapedString(userName).C_String());

		outTemp[0]=hardDriveFilename;
		outTemp[1]=hardDriveData;
		outTemp[2]=hardDriveHash;
		outLengths[0]=(int)strlen(hardDriveFilename);
		outLengths[1]=hardDriveDataLength;
		outLengths[2]=HASH_LENGTH;
		formats[0]=PQEXECPARAM_FORMAT_TEXT;
		RakAssert(formats[1]==PQEXECPARAM_FORMAT_BINARY);
		RakAssert(formats[2]==PQEXECPARAM_FORMAT_BINARY);
		
		// Upload the new file
		//sqlCommandMutex.Lock();
		uploadResult = PQexecParams(pgConn, query,3,0,outTemp,outLengths,formats,PQEXECPARAM_FORMAT_BINARY);
		//sqlCommandMutex.Unlock();
		if( !uploadResult )
		{
			// Libpq had a problem inserting the file to the table. Most likely due to it running out of
			// memory or a buffer being too small.
			RakAssert( 0 );
			newFiles.Clear();
			PQclear(result);
			return false;
		}
		
		if (IsResultSuccessful(uploadResult, true)==false)
		{
			newFiles.Clear();
			PQclear(uploadResult);
			PQclear(result);
			return false;
		}

		// Clear the upload result
		PQclear(uploadResult);
	}
	
	printf("DB COMMIT\n");

	// If ExecuteBlockingCommand fails then it does a rollback
	//sqlCommandMutex.Lock();
	if (ExecuteBlockingCommand("COMMIT;", &result, true)==false)
	{
		//sqlCommandMutex.Unlock();
		printf("COMMIT Failed!\n");
		PQclear(result);
		return false;
	}
	//sqlCommandMutex.Unlock();
	PQclear(result);

	printf("COMMIT Success\n");

	return true;
}
const char *AutopatcherPostgreRepository::GetLastError(void) const
{
	return PostgreSQLInterface::GetLastError();
}
unsigned int AutopatcherPostgreRepository::GetFilePart( const char *filename, unsigned int startReadBytes, unsigned int numBytesToRead, void *preallocatedDestination, FileListNodeContext context)
{
	PGresult *result;
	char query[512];
	char *content;
	int contentLength;

	// Seems that substring is 1 based for its index, so add 1 to startReadBytes
	sprintf(query, "SELECT substring(content from %i for %i) FROM FileVersionHistory WHERE fileId=%i;", startReadBytes+1,numBytesToRead,context.fileId);
	//sqlCommandMutex.Lock();
	result = PQexecParams(pgConn, query,0,0,0,0,0,PQEXECPARAM_FORMAT_BINARY);
	if (IsResultSuccessful(result, false)==false)
	{
		//sqlCommandMutex.Unlock();
		PQclear(result);
		return 0;
	}
	//sqlCommandMutex.Unlock();
	content = PQgetvalue(result, 0, 0);
	contentLength=PQgetlength(result, 0, 0);
	memcpy(preallocatedDestination,content,contentLength);
	PQclear(result);
	return contentLength;
}
