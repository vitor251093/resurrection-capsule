/// \file
/// \brief An implementation of TitleValidationDBSpec.h to use PostgreSQL. Title validation is useful for checking server logins and cd keys.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __TITLE_VALIDATION_POSTGRESQL_REPOSITORY_H
#define __TITLE_VALIDATION_POSTGRESQL_REPOSITORY_H

#include "TitleValidationDBSpec.h"
#include "PostgreSQLInterface.h"
#include "FunctionThread.h"

struct pg_conn;
typedef struct pg_conn PGconn;

struct pg_result;
typedef struct pg_result PGresult;

class TitleValidation_PostgreSQL;
class TitleValidationDBCBInterface;

class TitleValidationFunctor : public RakNet::Functor
{
public:
	TitleValidationFunctor() {titleValidationServer=0; callback=0; dbQuerySuccess=false;}
	static char* AllocBytes(int numBytes);
	static void FreeBytes(char *data);

	/// [in] Set this to a pointer to your title validation server, so that the Process() function can access the database classes
	TitleValidation_PostgreSQL *titleValidationServer;
	/// [in] Set this to a pointer to a callback if you wish to get notification calls. Otherwise you will have to override HandleResult()
	TitleValidationDBCBInterface *callback;
	/// [out] Whether or not the database query succeeded
	bool dbQuerySuccess;
};

class AddTitle_PostgreSQLImpl : public TitleValidationDBSpec::AddTitle_Data, public TitleValidationFunctor
{
public:
	/// Does SQL Processing
	virtual void Process(void *context);

	/// Calls the callback specified by TitleValidationFunctor::callback (which you set)
	/// or TitleValidation_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);

	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static AddTitle_PostgreSQLImpl* Alloc(void);
	static void Free(AddTitle_PostgreSQLImpl *s);
};

class GetTitles_PostgreSQLImpl : public TitleValidationDBSpec::GetTitles_Data, public TitleValidationFunctor
{
public:
	/// Does SQL Processing
	virtual void Process(void *context);

	/// Calls the callback specified by TitleValidationFunctor::callback (which you set)
	/// or TitleValidation_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);

	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetTitles_PostgreSQLImpl* Alloc(void);
	static void Free(GetTitles_PostgreSQLImpl *s);
};

class UpdateUserKey_PostgreSQLImpl : public TitleValidationDBSpec::UpdateUserKey_Data, public TitleValidationFunctor
{
public:
	/// Does SQL Processing
	virtual void Process(void *context);

	/// Calls the callback specified by TitleValidationFunctor::callback (which you set)
	/// or TitleValidation_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);

	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static UpdateUserKey_PostgreSQLImpl* Alloc(void);
	static void Free(UpdateUserKey_PostgreSQLImpl *s);
};


class GetUserKeys_PostgreSQLImpl : public TitleValidationDBSpec::GetUserKeys_Data, public TitleValidationFunctor
{
public:
	/// Does SQL Processing
	virtual void Process(void *context);

	/// Calls the callback specified by TitleValidationFunctor::callback (which you set)
	/// or TitleValidation_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);

	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetUserKeys_PostgreSQLImpl* Alloc(void);
	static void Free(GetUserKeys_PostgreSQLImpl *s);
};

class ValidateUserKey_PostgreSQLImpl : public TitleValidationDBSpec::ValidateUserKey_Data, public TitleValidationFunctor
{
public:
	/// Does SQL Processing
	virtual void Process(void *context);

	/// Calls the callback specified by TitleValidationFunctor::callback (which you set)
	/// or TitleValidation_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);

	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static ValidateUserKey_PostgreSQLImpl* Alloc(void);
	static void Free(ValidateUserKey_PostgreSQLImpl *s);
};

/// If you want to forward Functor::HandleResult() the output to a callback (which is what it does by default), you can use this
class RAK_DLL_EXPORT TitleValidationDBCBInterface
{
public:
	virtual void AddTitle_CB(AddTitle_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetTitles_CB(GetTitles_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void UpdateUserKey_CB(UpdateUserKey_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetUserKeys_CB(GetUserKeys_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void ValidateUserKey_CB(ValidateUserKey_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
};

/// Provides a central class for all TitleValidation functionality using the PostgreSQL interface
/// If you are using more than one class that uses functionThreads, you should maintain this separately and call AssignFunctionThread() to avoid unnecessary threads.
class RAK_DLL_EXPORT TitleValidation_PostgreSQL :
	public PostgreSQLInterface, // Gives the class C++ functions to access PostgreSQL
	public RakNet::FunctionThreadDependentClass // Adds a function thread member.
{
public:
	TitleValidation_PostgreSQL();
	~TitleValidation_PostgreSQL();

	/// Create the tables used by the ranking server, for all applications.  Call this first.
	/// I recommend using UTF8 for the database encoding within PostgreSQL if you are going to store binary data
	/// \return True on success, false on failure.
	bool CreateTitleValidationTables(void);

	/// Destroy the tables used by the ranking server.  Don't call this unless you don't want to use the ranking server anymore, or are testing.
	/// \return True on success, false on failure.
	bool DestroyTitleValidationTables(void);

	/// Push one of the above *_PostgreSQLImpl functors to run.
	/// \param functor A structure allocated on the HEAP (using new) with the input parameters filled in.
	virtual void PushFunctor(TitleValidationFunctor *functor, void *context=0);

	/// Assigns a callback to get the results of processing.
	/// \param[in] A structure allocated on the HEAP (using new) with the input parameters filled in.
	void AssignCallback(TitleValidationDBCBInterface *cb);

protected:
	friend class AddTitle_PostgreSQLImpl;
	friend class GetTitles_PostgreSQLImpl;
	friend class UpdateUserKey_PostgreSQLImpl;
	friend class GetUserKeys_PostgreSQLImpl;
	friend class ValidateUserKey_PostgreSQLImpl;

	TitleValidationDBCBInterface *titleValidationCallback;
};

#endif
