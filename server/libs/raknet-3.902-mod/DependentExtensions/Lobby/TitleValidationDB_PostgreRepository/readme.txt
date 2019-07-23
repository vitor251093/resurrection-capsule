Project: Title validation Database implemented with PostgreSQL as the backend

Description: This builds the title validation database implementation to a static library.

Dependencies: PostgreSQL version 8.2 (http://www.postgresql.org/download/), installed to C:\Program Files\PostgreSQL\8.2. If your installation directory is different, update it at
C/C++ / General / Additional Include Directories
and under
Build Events / Post Build Events
copy "C:\Program Files\PostgreSQL\8.1\bin\*.dll" .\Debug
to whatever your path is.

Related projects: TitleValidationDB_PostgreSQLTest

For help and support, please visit http://www.jenkinssoftware.com