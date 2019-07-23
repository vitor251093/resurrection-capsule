/// \internal
/// \brief Hashing container
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __HASH_H
#define __HASH_H 

#include "RakAssert.h"
#include <string.h> // memmove
#include "Export.h"
#include "RakMemoryOverride.h"
#include "RakString.h"

/// The namespace DataStructures was only added to avoid compiler errors for commonly named data structures
/// As these data structures are stand-alone, you can use them outside of RakNet for your own projects if you wish.
namespace DataStructures
{
	struct StringKeyedHashIndex
	{
		unsigned int primaryIndex;
		unsigned int secondaryIndex;
		bool IsInvalid(void) const {return primaryIndex==(unsigned int) -1;}
		void SetInvalid(void) {primaryIndex=(unsigned int) -1;}
	};

	/// \brief Using a string as a identifier for a node, store an allocated pointer to that node
	template <class data_type, unsigned int HASH_SIZE>
	class RAK_DLL_EXPORT StringKeyedHash
	{	
	public:
		/// Default constructor
		StringKeyedHash();

		// Destructor
		~StringKeyedHash();

		void Push(const char *key, const data_type &input, const char *file, unsigned int line );
		data_type* Peek(const char *key );
		bool Pop(data_type& out, const char *key, const char *file, unsigned int line );
		bool RemoveAtIndex(StringKeyedHashIndex index, const char *file, unsigned int line );
		StringKeyedHashIndex GetIndexOf(const char *key);
		data_type& ItemAtIndex(const StringKeyedHashIndex &index);
		RakNet::RakString KeyAtIndex(const StringKeyedHashIndex &index);
		void GetItemList(DataStructures::List<data_type> &outputList,const char *file, unsigned int line);
		unsigned int GetHashIndex(const char *str);

		/// \brief Clear the list		
		void Clear( const char *file, unsigned int line );

		struct Node
		{
			Node(const char *strIn, const data_type &_data) {string=strIn; data=_data;}
			RakNet::RakString string;
			data_type data;
			// Next in the list for this key
			Node *next;
		};

	protected:
		void ClearIndex(unsigned int index,const char *file, unsigned int line);
		Node **nodeList;
	};

	template <class data_type, unsigned int HASH_SIZE>
	StringKeyedHash<data_type, HASH_SIZE>::StringKeyedHash()
	{
		nodeList=0;
	}

	template <class data_type, unsigned int HASH_SIZE>
	StringKeyedHash<data_type, HASH_SIZE>::~StringKeyedHash()
	{
		Clear(__FILE__,__LINE__);
	}

	template <class data_type, unsigned int HASH_SIZE>
	void StringKeyedHash<data_type, HASH_SIZE>::Push(const char *key, const data_type &input, const char *file, unsigned int line )
	{
		unsigned long hashIndex = GetHashIndex(key);
		if (nodeList==0)
		{
			nodeList=RakNet::OP_NEW_ARRAY<Node *>(HASH_SIZE,file,line);
			memset(nodeList,0,sizeof(Node *)*HASH_SIZE);
		}

		Node *newNode=RakNet::OP_NEW_2<Node>(file,line,key,input);
		newNode->next=nodeList[hashIndex];
		nodeList[hashIndex]=newNode;
	}

	template <class data_type, unsigned int HASH_SIZE>
	data_type* StringKeyedHash<data_type, HASH_SIZE>::Peek(const char *key )
	{
		unsigned long hashIndex = GetHashIndex(key);
		Node *node = nodeList[hashIndex];
		while (node!=0)
		{
			if (node->string==key)
				return &node->data;
			node=node->next;
		}
		return 0;
	}

	template <class data_type, unsigned int HASH_SIZE>
	bool StringKeyedHash<data_type, HASH_SIZE>::Pop(data_type& out, const char *key, const char *file, unsigned int line )
	{
		unsigned long hashIndex = GetHashIndex(key);
		Node *node = nodeList[hashIndex];
		if (node==0)
			return false;
		if (node->next==0)
		{
			// Only one item.
			if (node->string==key)
			{
				// Delete last item
				out=node->data;
				ClearIndex(hashIndex,__FILE__,__LINE__);
				return true;
			}
			else
			{
				// Single item doesn't match
				return false;
			}
		}
		else if (node->string==key)
		{
			// First item does match, but more than one item
			out=node->data;
			nodeList[hashIndex]=node->next;
			RakNet::OP_DELETE(node,file,line);
			return true;
		}

		Node *last=node;
		node=node->next;

		while (node!=0)
		{
			// First item does not match, but subsequent item might
			if (node->string==key)
			{
				out=node->data;
				// Skip over subsequent item
				last->next=node->next;
				// Delete existing item
				RakNet::OP_DELETE(node,file,line);
				return true;
			}
			last=node;
			node=node->next;
		}
		return false;
	}

	template <class data_type, unsigned int HASH_SIZE>
	bool StringKeyedHash<data_type, HASH_SIZE>::RemoveAtIndex(StringKeyedHashIndex index, const char *file, unsigned int line )
	{
		if (index.IsInvalid())
			return false;

		Node *node = nodeList[index.primaryIndex];
		if (node==0)
			return false;
		if (node->next==0)
		{
			// Delete last item
			ClearIndex(index.primaryIndex);
			return true;
		}
		else if (index.secondaryIndex==0)
		{
			// First item does match, but more than one item
			nodeList[index.primaryIndex]=node->next;
			RakNet::OP_DELETE(node,file,line);
			return true;
		}

		Node *last=node;
		node=node->next;
		--index.secondaryIndex;

		while (index.secondaryIndex!=0)
		{
			last=node;
			node=node->next;
			--index.secondaryIndex;
		}

		// Skip over subsequent item
		last->next=node->next;
		// Delete existing item
		RakNet::OP_DELETE(node,file,line);
		return true;
	}

	template <class data_type, unsigned int HASH_SIZE>
	StringKeyedHashIndex StringKeyedHash<data_type, HASH_SIZE>::GetIndexOf(const char *key)
	{
		if (nodeList==0)
		{
			StringKeyedHashIndex temp;
			temp.SetInvalid();
			return temp;
		}
		StringKeyedHashIndex idx;
		idx.primaryIndex=GetHashIndex(key);
		Node *node = nodeList[idx.primaryIndex];
		if (node==0)
		{
			idx.SetInvalid();
			return idx;
		}
		idx.secondaryIndex=0;
		while (node!=0)
		{
			if (node->string==key)
			{
				return idx;
			}
			node=node->next;
			idx.secondaryIndex++;
		}

		idx.SetInvalid();
		return idx;
	}

	template <class data_type, unsigned int HASH_SIZE>
	data_type& StringKeyedHash<data_type, HASH_SIZE>::ItemAtIndex(const StringKeyedHashIndex &index)
	{
		Node *node = nodeList[index.primaryIndex];
		RakAssert(node);
		unsigned int i;
		for (i=0; i < index.secondaryIndex; i++)
		{
			node=node->next;
			RakAssert(node);
		}
		return node->data;
	}

	template <class data_type, unsigned int HASH_SIZE>
	RakNet::RakString StringKeyedHash<data_type, HASH_SIZE>::KeyAtIndex(const StringKeyedHashIndex &index)
	{
		Node *node = nodeList[index.primaryIndex];
		RakAssert(node);
		unsigned int i;
		for (i=0; i < index.secondaryIndex; i++)
		{
			node=node->next;
			RakAssert(node);
		}
		return node->string;
	}

	template <class data_type, unsigned int HASH_SIZE>
	void StringKeyedHash<data_type, HASH_SIZE>::Clear(const char *file, unsigned int line)
	{
		if (nodeList)
		{
			unsigned int i;
			for (i=0; i < HASH_SIZE; i++)
				ClearIndex(i,file,line);
			RakNet::OP_DELETE_ARRAY(nodeList,file,line);
			nodeList=0;
		}
	}

	template <class data_type, unsigned int HASH_SIZE>
	void StringKeyedHash<data_type, HASH_SIZE>::ClearIndex(unsigned int index,const char *file, unsigned int line)
	{
		Node *node = nodeList[index];
		Node *next;
		while (node)
		{
			next=node->next;
			RakNet::OP_DELETE(node,file,line);
			node=next;
		}
		nodeList[index]=0;
	}

	template <class data_type, unsigned int HASH_SIZE>
	void StringKeyedHash<data_type, HASH_SIZE>::GetItemList(DataStructures::List<data_type> &outputList,const char *file, unsigned int line)
	{
		if (nodeList==0)
			return;
		outputList.Clear(false,__FILE__,__LINE__);

		Node *node;
		unsigned int i;
		for (i=0; i < HASH_SIZE; i++)
		{
			if (nodeList[i])
			{
				node=nodeList[i];
				while (node)
				{
					outputList.Push(node->data,file,line);
					node=node->next;
				}
			}
		}
	}

	template <class data_type, unsigned int HASH_SIZE>
	unsigned int StringKeyedHash<data_type, HASH_SIZE>::GetHashIndex(const char *str)
	{
		return RakNet::RakString::ToInteger(str) % HASH_SIZE;
	}
}
#endif
