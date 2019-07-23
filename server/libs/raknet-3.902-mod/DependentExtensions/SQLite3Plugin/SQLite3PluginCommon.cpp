#include "SQLite3PluginCommon.h"


SQLite3Table::SQLite3Table()
{

}
SQLite3Table::~SQLite3Table()
{
	rows.ClearPointers(true,__FILE__,__LINE__);
}

void SQLite3Table::Serialize(RakNet::BitStream *bitStream)
{
	bitStream->Write(columnNames.GetSize());
	DataStructures::DefaultIndexType idx1, idx2;
	for (idx1=0; idx1 < columnNames.GetSize(); idx1++)
		bitStream->Write(columnNames[idx1]);
	bitStream->Write(rows.GetSize());
	for (idx1=0; idx1 < rows.GetSize(); idx1++)
	{
		for (idx2=0; idx2 < rows[idx1]->entries.GetSize(); idx2++)
		{
			bitStream->Write(rows[idx1]->entries[idx2]);
		}
	}
}
void SQLite3Table::Deserialize(RakNet::BitStream *bitStream)
{
	rows.ClearPointers(true,__FILE__,__LINE__);
	columnNames.Clear(true , __FILE__, __LINE__ );

	DataStructures::DefaultIndexType numColumns, numRows;
	bitStream->Read(numColumns);
	DataStructures::DefaultIndexType idx1,idx2;
	RakNet::RakString inputStr;
	for (idx1=0; idx1 < numColumns; idx1++)
	{
		bitStream->Read(inputStr);
		columnNames.Push(inputStr, __FILE__, __LINE__ );
	}
	bitStream->Read(numRows);
	for (idx1=0; idx1 < numRows; idx1++)
	{
		SQLite3Row *row = RakNet::OP_NEW<SQLite3Row>(__FILE__,__LINE__);
		rows.Push(row,__FILE__,__LINE__);
		for (idx2=0; idx2 < numColumns; idx2++)
		{
			bitStream->Read(inputStr);
			row->entries.Push(inputStr, __FILE__, __LINE__ );
		}
	}
}