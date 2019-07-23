#ifndef __MY_SQL_INTERFACE_H
#define __MY_SQL_INTERFACE_H

#include "RakString.h"

struct st_mysql_res;
struct st_mysql;

class MySQLInterface
{
public:
	MySQLInterface();
	virtual ~MySQLInterface();

	/// Calls mysql_real_connect with the implicit mySqlConnection 
	bool Connect (const char *host,
		const char *user,
		const char *passwd,
		const char *db,
		unsigned int port,
		const char *unix_socket,
		unsigned long clientflag);

	/// Disconnect from the database
	void Disconnect(void);

	/// Returns if we are connected to the database
	bool IsConnected(void) const;

	/// If any of the above functions fail, the error string is stored internally.  Call this to get it.
	virtual const char *GetLastError(void) const;

	/// Returns the result of SELECT LOCALTIMESTAMP
	char *GetLocalTimestamp(void);

protected:
	// Pass queries to the server
	bool ExecuteBlockingCommand(const char *command);
	bool ExecuteBlockingCommand(const char *command, st_mysql_res **result, bool rollbackOnFailure = false);
	bool ExecuteQueryReadInt (const char * query, int *value);
	void Commit(void);
	void Rollback(void);
	RakNet::RakString GetEscapedString(const char *input) const;

	st_mysql *mySqlConnection;
	char lastError[1024];

};

#endif
