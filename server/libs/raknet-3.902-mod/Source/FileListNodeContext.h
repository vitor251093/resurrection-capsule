/// \file FileListNodeContext.h
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.

#ifndef __FILE_LIST_NODE_CONTEXT_H
#define __FILE_LIST_NODE_CONTEXT_H

#include "BitStream.h"

struct FileListNodeContext
{
	FileListNodeContext() {}
	FileListNodeContext(unsigned char o, unsigned int f) : op(o), fileId(f) {}
	~FileListNodeContext() {}

	unsigned char op;
	unsigned int fileId;
};

inline RakNet::BitStream& operator<<(RakNet::BitStream& out, FileListNodeContext& in)
{
	out.Write(in.op);
	out.Write(in.fileId);
	return out;
}
inline RakNet::BitStream& operator>>(RakNet::BitStream& in, FileListNodeContext& out)
{
	in.Read(out.op);
	bool success = in.Read(out.fileId);
	assert(success);
	return in;
}

#endif
