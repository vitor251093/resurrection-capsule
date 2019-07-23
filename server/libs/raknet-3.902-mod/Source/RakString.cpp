#include "RakString.h"
#include "RakAssert.h"
#include "RakMemoryOverride.h"
#include "BitStream.h"
#include <stdarg.h>
#include <string.h>
#include "LinuxStrings.h"
#include "StringCompressor.h"
#include "SimpleMutex.h"

using namespace RakNet;

//DataStructures::MemoryPool<RakString::SharedString> RakString::pool;
unsigned int RakString::nPos=(unsigned int) -1;
RakString::SharedString RakString::emptyString={0,0,0,(char*) "",(char*) ""};
//RakString::SharedString *RakString::sharedStringFreeList=0;
//unsigned int RakString::sharedStringFreeListAllocationCount=0;
DataStructures::List<RakString::SharedString*> RakString::freeList;

class RakStringCleanup
{
public:
	~RakStringCleanup()
	{
		RakNet::RakString::FreeMemoryNoMutex();
	}
};

static RakStringCleanup cleanup;

SimpleMutex& GetPoolMutex(void)
{
	static SimpleMutex poolMutex;
	return poolMutex;
}

int RakString::RakStringComp( RakString const &key, RakString const &data )
{
	return key.StrCmp(data);
}

RakString::RakString()
{
	sharedString=&emptyString;
}
RakString::RakString( RakString::SharedString *_sharedString )
{
	sharedString=_sharedString;
}
RakString::RakString(char input)
{
	char str[2];
	str[0]=input;
	str[1]=0;
	Assign(str);
}
RakString::RakString(unsigned char input)
{
	char str[2];
	str[0]=(char) input;
	str[1]=0;
	Assign(str);
}
RakString::RakString(const unsigned char *format, ...){
	va_list ap;
	va_start(ap, format);
	Assign((const char*) format,ap);
}
RakString::RakString(const char *format, ...){
	va_list ap;
	va_start(ap, format);
	Assign(format,ap);
}
RakString::RakString( const RakString & rhs)
{
	if (rhs.sharedString==&emptyString)
	{
		sharedString=&emptyString;
		return;
	}

	rhs.sharedString->refCountMutex->Lock();
	if (rhs.sharedString->refCount==0)
	{
		sharedString=&emptyString;
	}
	else
	{
		rhs.sharedString->refCount++;
		sharedString=rhs.sharedString;
	}
	rhs.sharedString->refCountMutex->Unlock();
}
RakString::~RakString()
{
	Free();
}
RakString& RakString::operator = ( const RakString& rhs )
{
	Free();
	if (rhs.sharedString==&emptyString)
		return *this;

	rhs.sharedString->refCountMutex->Lock();
	if (rhs.sharedString->refCount==0)
	{
		sharedString=&emptyString;
	}
	else
	{
		sharedString=rhs.sharedString;
		sharedString->refCount++;
	}
	rhs.sharedString->refCountMutex->Unlock();
	return *this;
}
RakString& RakString::operator = ( const char *str )
{
	Free();
	Assign(str);
	return *this;
}
RakString& RakString::operator = ( char *str )
{
	return operator = ((const char*)str);
}
RakString& RakString::operator = ( const unsigned char *str )
{
	return operator = ((const char*)str);
}
RakString& RakString::operator = ( char unsigned *str )
{
	return operator = ((const char*)str);
}
RakString& RakString::operator = ( const char c )
{
	char buff[2];
	buff[0]=c;
	buff[1]=0;
	return operator = ((const char*)buff);
}
void RakString::Realloc(SharedString *sharedString, size_t bytes)
{
	if (bytes<=sharedString->bytesUsed)
		return;
	RakAssert(bytes>0);
	size_t oldBytes = sharedString->bytesUsed;
	size_t newBytes;
	const size_t smallStringSize = 128-sizeof(unsigned int)-sizeof(size_t)-sizeof(char*)*2;
	newBytes = GetSizeToAllocate(bytes);
	if (oldBytes <=(size_t) smallStringSize && newBytes > (size_t) smallStringSize)
	{
		sharedString->bigString=(char*) rakMalloc_Ex(newBytes, __FILE__, __LINE__);
		strcpy(sharedString->bigString, sharedString->smallString);
		sharedString->c_str=sharedString->bigString;
	}
	else if (oldBytes > smallStringSize)
	{
		sharedString->bigString=(char*) rakRealloc_Ex(sharedString->bigString,newBytes, __FILE__, __LINE__);
		sharedString->c_str=sharedString->bigString;
	}
	sharedString->bytesUsed=newBytes;
}
RakString& RakString::operator +=( const RakString& rhs)
{
	if (rhs.IsEmpty())
		return *this;

	if (IsEmpty())
	{
		return operator=(rhs);
	}
	else
	{
		Clone();
		size_t strLen=rhs.GetLength()+GetLength()+1;
		Realloc(sharedString, strLen+GetLength());
		strcat(sharedString->c_str,rhs.C_String());
	}
	return *this;
}
RakString& RakString::operator +=( const char *str )
{
	if (str==0 || str[0]==0)
		return *this;

	if (IsEmpty())
	{
		Assign(str);
	}
	else
	{
		Clone();
		size_t strLen=strlen(str)+GetLength()+1;
		Realloc(sharedString, strLen);
		strcat(sharedString->c_str,str);
	}
	return *this;
}
RakString& RakString::operator +=( char *str )
{
	return operator += ((const char*)str);
}
RakString& RakString::operator +=( const unsigned char *str )
{
	return operator += ((const char*)str);
}
RakString& RakString::operator +=( unsigned char *str )
{
	return operator += ((const char*)str);
}
RakString& RakString::operator +=( const char c )
{
	char buff[2];
	buff[0]=c;
	buff[1]=0;
	return operator += ((const char*)buff);
}
unsigned char RakString::operator[] ( const unsigned int position ) const
{
	RakAssert(position<GetLength());
	return sharedString->c_str[position];
}
bool RakString::operator==(const RakString &rhs) const
{
	return strcmp(sharedString->c_str,rhs.sharedString->c_str)==0;
}
bool RakString::operator==(const char *str) const
{
	return strcmp(sharedString->c_str,str)==0;
}
bool RakString::operator==(char *str) const
{
	return strcmp(sharedString->c_str,str)==0;
}
bool RakString::operator < ( const RakString& right ) const
{
	return strcmp(sharedString->c_str,right.C_String()) < 0;
}
bool RakString::operator <= ( const RakString& right ) const
{
	return strcmp(sharedString->c_str,right.C_String()) <= 0;
}
bool RakString::operator > ( const RakString& right ) const
{
	return strcmp(sharedString->c_str,right.C_String()) > 0;
}
bool RakString::operator >= ( const RakString& right ) const
{
	return strcmp(sharedString->c_str,right.C_String()) >= 0;
}
bool RakString::operator!=(const RakString &rhs) const
{
	return strcmp(sharedString->c_str,rhs.sharedString->c_str)!=0;
}
bool RakString::operator!=(const char *str) const
{
	return strcmp(sharedString->c_str,str)!=0;
}
bool RakString::operator!=(char *str) const
{
	return strcmp(sharedString->c_str,str)!=0;
}
const RakNet::RakString operator+(const RakNet::RakString &lhs, const RakNet::RakString &rhs)
{
	if (lhs.IsEmpty() && rhs.IsEmpty())
	{
		return RakString(&RakString::emptyString);
	}
	if (lhs.IsEmpty())
	{
		rhs.sharedString->refCountMutex->Lock();
		if (rhs.sharedString->refCount==0)
		{
			rhs.sharedString->refCountMutex->Unlock();
			lhs.sharedString->refCountMutex->Lock();
			lhs.sharedString->refCount++;
			lhs.sharedString->refCountMutex->Unlock();
			return RakString(lhs.sharedString);
		}
		else
		{
			rhs.sharedString->refCount++;
			rhs.sharedString->refCountMutex->Unlock();
			return RakString(rhs.sharedString);
		}
		// rhs.sharedString->refCountMutex->Unlock();
	}
	if (rhs.IsEmpty())
	{
		lhs.sharedString->refCountMutex->Lock();
		lhs.sharedString->refCount++;
		lhs.sharedString->refCountMutex->Unlock();
		return RakString(lhs.sharedString);
	}

	size_t len1 = lhs.GetLength();
	size_t len2 = rhs.GetLength();
	size_t allocatedBytes = len1 + len2 + 1;
	allocatedBytes = RakString::GetSizeToAllocate(allocatedBytes);
	RakString::SharedString *sharedString;

	RakString::LockMutex();
	// sharedString = RakString::pool.Allocate( __FILE__, __LINE__ );
	if (RakString::freeList.Size()==0)
	{
		//RakString::sharedStringFreeList=(RakString::SharedString*) rakRealloc_Ex(RakString::sharedStringFreeList,(RakString::sharedStringFreeListAllocationCount+1024)*sizeof(RakString::SharedString), __FILE__, __LINE__);
		unsigned i;
		for (i=0; i < 128; i++)
		{
		//	RakString::freeList.Insert(RakString::sharedStringFreeList+i+RakString::sharedStringFreeListAllocationCount);
			RakString::SharedString *ss;
			ss = (RakString::SharedString*) rakMalloc_Ex(sizeof(RakString::SharedString), __FILE__, __LINE__);
			ss->refCountMutex=RakNet::OP_NEW<SimpleMutex>(__FILE__,__LINE__);
			RakString::freeList.Insert(ss, __FILE__, __LINE__);

		}
		//RakString::sharedStringFreeListAllocationCount+=1024;
	}
	sharedString = RakString::freeList[RakString::freeList.Size()-1];
	RakString::freeList.RemoveAtIndex(RakString::freeList.Size()-1);
	RakString::UnlockMutex();

	const int smallStringSize = 128-sizeof(unsigned int)-sizeof(size_t)-sizeof(char*)*2;
	sharedString->bytesUsed=allocatedBytes;
	sharedString->refCount=1;
	if (allocatedBytes <= (size_t) smallStringSize)
	{
		sharedString->c_str=sharedString->smallString;
	}
	else
	{
		sharedString->bigString=(char*)rakMalloc_Ex(sharedString->bytesUsed, __FILE__, __LINE__);
		sharedString->c_str=sharedString->bigString;
	}

	strcpy(sharedString->c_str, lhs);
	strcat(sharedString->c_str, rhs);

	return RakString(sharedString);
}
const char * RakString::ToLower(void)
{
	Clone();

	size_t strLen = strlen(sharedString->c_str);
	unsigned i;
	for (i=0; i < strLen; i++)
		sharedString->c_str[i]=ToLower(sharedString->c_str[i]);
	return sharedString->c_str;
}
const char * RakString::ToUpper(void)
{
	Clone();

	size_t strLen = strlen(sharedString->c_str);
	unsigned i;
	for (i=0; i < strLen; i++)
		sharedString->c_str[i]=ToUpper(sharedString->c_str[i]);
	return sharedString->c_str;
}
void RakString::Set(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	Clear();
	Assign(format,ap);
}
bool RakString::IsEmpty(void) const
{
	return sharedString==&emptyString;
}
size_t RakString::GetLength(void) const
{
	return strlen(sharedString->c_str);
}
void RakString::Replace(unsigned index, unsigned count, unsigned char c)
{
	RakAssert(index+count < GetLength());
	Clone();
	unsigned countIndex=0;
	while (countIndex<count)
	{
		sharedString->c_str[index]=c;
		index++;
		countIndex++;
	}
}
void RakString::SetChar( unsigned index, unsigned char c )
{
	RakAssert(index < GetLength());
	Clone();
	sharedString->c_str[index]=c;
}
void RakString::SetChar( unsigned index, RakNet::RakString s )
{
	RakAssert(index < GetLength());
	Clone();
	RakNet::RakString firstHalf = SubStr(0, index);
	RakNet::RakString secondHalf = SubStr(index+1, (unsigned int)-1);
	*this = firstHalf;
	*this += s;
	*this += secondHalf;
}

size_t RakString::Find(const char *stringToFind,size_t pos)
{
	size_t len=GetLength();
	if (pos>=len || stringToFind==0 || stringToFind[0]==0)
	{
		return nPos;
	}
	size_t matchLen= strlen(stringToFind);
	size_t matchPos=0;
	size_t iStart=0;

	for (size_t i=pos;i<len;i++)
	{
		if (stringToFind[matchPos]==sharedString->c_str[i])
		{
			if(matchPos==0)
			{
				iStart=i;
			}
			matchPos++;
		}
		else
		{
			matchPos=0;
		}

		if (matchPos>=matchLen)
		{
			return iStart;
		}
	}

	return nPos;
}

void RakString::Truncate(unsigned length)
{
	if (length < GetLength())
	{
		SetChar(length, 0);
	}
}
RakString RakString::SubStr(unsigned int index, unsigned int count) const
{
	size_t length = GetLength();
	if (index >= length || count==0)
		return RakString();
	RakString copy;
	size_t numBytes = length-index;
	if (count < numBytes)
		numBytes=count;
	copy.Allocate(numBytes+1);
	size_t i;
	for (i=0; i < numBytes; i++)
		copy.sharedString->c_str[i]=sharedString->c_str[index+i];
	copy.sharedString->c_str[i]=0;
	return copy;
}
void RakString::Erase(unsigned int index, unsigned int count)
{
	size_t len = GetLength();
	RakAssert(index+count <= len);
        
	Clone();
	unsigned i;
	for (i=index; i < len-count; i++)
	{
		sharedString->c_str[i]=sharedString->c_str[i+count];
	}
	sharedString->c_str[i]=0;
}
void RakString::TerminateAtLastCharacter(char c)
{
	int i, len=(int) GetLength();
	for (i=len-1; i >= 0; i--)
	{
		if (sharedString->c_str[i]==c)
		{
			Clone();
			sharedString->c_str[i]=0;
			return;
		}
	}
}
void RakString::TerminateAtFirstCharacter(char c)
{
	unsigned int i, len=(unsigned int) GetLength();
	for (i=0; i < len; i++)
	{
		if (sharedString->c_str[i]==c)
		{
			Clone();
			sharedString->c_str[i]=0;
			return;
		}
	}
}
void RakString::RemoveCharacter(char c)
{
	if (c==0)
		return;

	unsigned int readIndex, writeIndex=0;
	for (readIndex=0; sharedString->c_str[readIndex]; readIndex++)
	{
		if (sharedString->c_str[readIndex]!=c)
			sharedString->c_str[writeIndex++]=sharedString->c_str[readIndex];
		else
			Clone();
	}
	sharedString->c_str[writeIndex]=0;
}
int RakString::StrCmp(const RakString &rhs) const
{
	return strcmp(sharedString->c_str, rhs);
}
int RakString::StrICmp(const RakString &rhs) const
{
	return _stricmp(sharedString->c_str, rhs);
}
void RakString::Printf(void)
{
	RAKNET_DEBUG_PRINTF("%s", sharedString->c_str);
}
void RakString::FPrintf(FILE *fp)
{
	fprintf(fp,"%s", sharedString->c_str);
}
bool RakString::IPAddressMatch(const char *IP)
{
	unsigned characterIndex;

	if ( IP == 0 || IP[ 0 ] == 0 || strlen( IP ) > 15 )
		return false;

	characterIndex = 0;

#ifdef _MSC_VER
#pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
#endif
	while ( true )
	{
		if (sharedString->c_str[ characterIndex ] == IP[ characterIndex ] )
		{
			// Equal characters
			if ( IP[ characterIndex ] == 0 )
			{
				// End of the string and the strings match

				return true;
			}

			characterIndex++;
		}

		else
		{
			if ( sharedString->c_str[ characterIndex ] == 0 || IP[ characterIndex ] == 0 )
			{
				// End of one of the strings
				break;
			}

			// Characters do not match
			if ( sharedString->c_str[ characterIndex ] == '*' )
			{
				// Domain is banned.
				return true;
			}

			// Characters do not match and it is not a *
			break;
		}
	}


	// No match found.
	return false;
}
bool RakString::ContainsNonprintableExceptSpaces(void) const
{
	size_t strLen = strlen(sharedString->c_str);
	unsigned i;
	for (i=0; i < strLen; i++)
	{
		if (sharedString->c_str[i] < ' ' || sharedString->c_str[i] >126)
			return true;
	}
	return false;
}
bool RakString::IsEmailAddress(void) const
{
	if (IsEmpty())
		return false;
	size_t strLen = strlen(sharedString->c_str);
	if (strLen < 6) // a@b.de
		return false;
	if (sharedString->c_str[strLen-4]!='.' && sharedString->c_str[strLen-3]!='.') // .com, .net., .org, .de
		return false;
	unsigned i;
	// Has non-printable?
	for (i=0; i < strLen; i++)
	{
		if (sharedString->c_str[i] <= ' ' || sharedString->c_str[i] >126)
			return false;
	}
	int atCount=0;
	for (i=0; i < strLen; i++)
	{
		if (sharedString->c_str[i]=='@')
		{
			atCount++;
		}
	}
	if (atCount!=1)
		return false;
	int dotCount=0;
	for (i=0; i < strLen; i++)
	{
		if (sharedString->c_str[i]=='.')
		{
			dotCount++;
		}
	}
	if (dotCount==0)
		return false;

	// There's more I could check, but this is good enough
	return true;
}
RakNet::RakString& RakString::URLEncode(void)
{
	RakString result;
	size_t strLen = strlen(sharedString->c_str);
	result.Allocate(strLen*3);
	char *output=result.sharedString->c_str;
	unsigned int outputIndex=0;
	unsigned i;
	char c;
	for (i=0; i < strLen; i++)
	{
		c=sharedString->c_str[i];
		if (
			(c<=47) ||
			(c>=58 && c<=64) ||
			(c>=91 && c<=96) ||
			(c>=123)
			)
		{
			RakNet::RakString tmp("%2X", c);
			output[outputIndex++]='%';
			output[outputIndex++]=tmp.sharedString->c_str[0];
			output[outputIndex++]=tmp.sharedString->c_str[1];
		}
		else
		{
			output[outputIndex++]=c;
		}
	}

	output[outputIndex]=0;

	*this = result;
	return *this;
}
RakNet::RakString& RakString::URLDecode(void)
{
	RakString result;
	size_t strLen = strlen(sharedString->c_str);
	result.Allocate(strLen);
	char *output=result.sharedString->c_str;
	unsigned int outputIndex=0;
	char c;
	char hexDigits[2];
	char hexValues[2];
	unsigned int i;
	for (i=0; i < strLen; i++)
	{
		c=sharedString->c_str[i];
		if (c=='%')
		{
			hexDigits[0]=sharedString->c_str[++i];
			hexDigits[1]=sharedString->c_str[++i];
			if (hexDigits[0]==' ')
				hexValues[0]=0;
			else if (hexDigits[0]>='A')
				hexValues[0]=hexDigits[0]-'A'+10;
			else
				hexValues[0]=hexDigits[0]-'0';
			if (hexDigits[1]>='A')
				hexValues[1]=hexDigits[1]-'A'+10;
			else
				hexValues[1]=hexDigits[1]-'0';
			output[outputIndex++]=hexValues[0]*16+hexValues[1];
		}
		else
		{
			output[outputIndex++]=c;
		}
	}

	output[outputIndex]=0;

	*this = result;
	return *this;
}
RakNet::RakString& RakString::SQLEscape(void)
{
	int strLen=(int)GetLength();
	int escapedCharacterCount=0;
	int index;
	for (index=0; index < strLen; index++)
	{
		if (sharedString->c_str[index]=='\'' ||
			sharedString->c_str[index]=='"' ||
			sharedString->c_str[index]=='\\')
			escapedCharacterCount++;
	}
	if (escapedCharacterCount==0)
		return *this;

	Clone();
	Realloc(sharedString, strLen+escapedCharacterCount);
	int writeIndex, readIndex;
	writeIndex = strLen+escapedCharacterCount;
	readIndex=strLen;
	while (readIndex>=0)
	{
		if (sharedString->c_str[readIndex]=='\'' ||
			sharedString->c_str[readIndex]=='"' ||
			sharedString->c_str[readIndex]=='\\')
		{
			sharedString->c_str[writeIndex--]=sharedString->c_str[readIndex--];
			sharedString->c_str[writeIndex--]='\\';
		}
		else
		{
			sharedString->c_str[writeIndex--]=sharedString->c_str[readIndex--];
		}
	}
	return *this;
}
RakNet::RakString& RakString::MakeFilePath(void)
{
	if (IsEmpty())
		return *this;

	RakNet::RakString fixedString = *this;
	fixedString.Clone();
	for (int i=0; fixedString.sharedString->c_str[i]; i++)
	{
#ifdef _WIN32
		if (fixedString.sharedString->c_str[i]=='/')
			fixedString.sharedString->c_str[i]='\\';
#else
		if (fixedString.sharedString->c_str[i]=='\\')
			fixedString.sharedString->c_str[i]='/';
#endif
	}

#ifdef _WIN32
	if (fixedString.sharedString->c_str[strlen(fixedString.sharedString->c_str)-1]!='\\')
	{
		fixedString+='\\';
	}
#else
	if (fixedString.sharedString->c_str[strlen(fixedString.sharedString->c_str)-1]!='/')
	{
		fixedString+='/';
	}
#endif

	if (fixedString!=*this)
		*this = fixedString;
	return *this;
}
void RakString::FreeMemory(void)
{
	LockMutex();
	FreeMemoryNoMutex();
	UnlockMutex();
}
void RakString::FreeMemoryNoMutex(void)
{
	for (unsigned int i=0; i < freeList.Size(); i++)
	{
		RakNet::OP_DELETE(freeList[i]->refCountMutex,__FILE__,__LINE__);
		rakFree_Ex(freeList[i], __FILE__, __LINE__ );
	}
	freeList.Clear(false, __FILE__, __LINE__);
}
void RakString::Serialize(BitStream *bs) const
{
	Serialize(sharedString->c_str, bs);
}
void RakString::Serialize(const char *str, BitStream *bs)
{
	unsigned short l = (unsigned short) strlen(str);
	bs->Write(l);
	bs->WriteAlignedBytes((const unsigned char*) str, (const unsigned int) l);
}
void RakString::SerializeCompressed(BitStream *bs, int languageId, bool writeLanguageId) const
{
	SerializeCompressed(C_String(), bs, languageId, writeLanguageId);
}
void RakString::SerializeCompressed(const char *str, BitStream *bs, int languageId, bool writeLanguageId)
{
	if (writeLanguageId)
		bs->WriteCompressed(languageId);
	stringCompressor->EncodeString(str,0xFFFF,bs,languageId);
}
bool RakString::Deserialize(BitStream *bs)
{
	Clear();

	bool b;
	unsigned short l;
	b=bs->Read(l);
	if (l>0)
	{
		Allocate(((unsigned int) l)+1);
		b=bs->ReadAlignedBytes((unsigned char*) sharedString->c_str, l);
		if (b)
			sharedString->c_str[l]=0;
		else
			Clear();
	}
	else
		bs->AlignReadToByteBoundary();
	return b;
}
bool RakString::Deserialize(char *str, BitStream *bs)
{
	bool b;
	unsigned short l;
	b=bs->Read(l);
	if (b && l>0)
		b=bs->ReadAlignedBytes((unsigned char*) str, l);

	if (b==false)
		str[0]=0;
	
	str[l]=0;
	return b;
}
bool RakString::DeserializeCompressed(BitStream *bs, bool readLanguageId)
{
	unsigned int languageId;
	if (readLanguageId)
		bs->ReadCompressed(languageId);
	else
		languageId=0;
	return stringCompressor->DecodeString(this,0xFFFF,bs,languageId);
}
bool RakString::DeserializeCompressed(char *str, BitStream *bs, bool readLanguageId)
{
	unsigned int languageId;
	if (readLanguageId)
		bs->ReadCompressed(languageId);
	else
		languageId=0;
	return stringCompressor->DecodeString(str,0xFFFF,bs,languageId);
}
const char *RakString::ToString(int64_t i)
{
	static int index=0;
	static char buff[64][64];
#if defined(_WIN32)
	sprintf(buff[index], "%I64d", i);
#else
	sprintf(buff[index], "%lld", (long long unsigned int) i);
#endif
	int lastIndex=index;
	if (++index==64)
		index=0;
	return buff[lastIndex];
}
const char *RakString::ToString(uint64_t i)
{
	static int index=0;
	static char buff[64][64];
#if defined(_WIN32)
	sprintf(buff[index], "%I64u", i);
#else
	sprintf(buff[index], "%llu", (long long unsigned int) i);
#endif
	int lastIndex=index;
	if (++index==64)
		index=0;
	return buff[lastIndex];
}
void RakString::Clear(void)
{
	Free();
}
void RakString::Allocate(size_t len)
{
	RakString::LockMutex();
	// sharedString = RakString::pool.Allocate( __FILE__, __LINE__ );
	if (RakString::freeList.Size()==0)
	{
		//RakString::sharedStringFreeList=(RakString::SharedString*) rakRealloc_Ex(RakString::sharedStringFreeList,(RakString::sharedStringFreeListAllocationCount+1024)*sizeof(RakString::SharedString), __FILE__, __LINE__);
		unsigned i;
		for (i=0; i < 128; i++)
		{
			//	RakString::freeList.Insert(RakString::sharedStringFreeList+i+RakString::sharedStringFreeListAllocationCount);
	//		RakString::freeList.Insert((RakString::SharedString*)rakMalloc_Ex(sizeof(RakString::SharedString), __FILE__, __LINE__), __FILE__, __LINE__);

			RakString::SharedString *ss;
			ss = (RakString::SharedString*) rakMalloc_Ex(sizeof(RakString::SharedString), __FILE__, __LINE__);
			ss->refCountMutex=RakNet::OP_NEW<SimpleMutex>(__FILE__,__LINE__);
			RakString::freeList.Insert(ss, __FILE__, __LINE__);
		}
		//RakString::sharedStringFreeListAllocationCount+=1024;
	}
	sharedString = RakString::freeList[RakString::freeList.Size()-1];
	RakString::freeList.RemoveAtIndex(RakString::freeList.Size()-1);
	RakString::UnlockMutex();

	const size_t smallStringSize = 128-sizeof(unsigned int)-sizeof(size_t)-sizeof(char*)*2;
	sharedString->refCount=1;
	if (len <= smallStringSize)
	{
		sharedString->bytesUsed=smallStringSize;
		sharedString->c_str=sharedString->smallString;
	}
	else
	{
		sharedString->bytesUsed=len<<1;
		sharedString->bigString=(char*)rakMalloc_Ex(sharedString->bytesUsed, __FILE__, __LINE__);
		sharedString->c_str=sharedString->bigString;
	}
}
void RakString::Assign(const char *str)
{
	if (str==0 || str[0]==0)
	{
		sharedString=&emptyString;
		return;
	}

	size_t len = strlen(str)+1;
	Allocate(len);
	memcpy(sharedString->c_str, str, len);
}
void RakString::Assign(const char *str, va_list ap)
{
	char stackBuff[512];
	if (_vsnprintf(stackBuff, 512, str, ap)!=-1
#ifndef _WIN32
		// Here Windows will return -1 if the string is too long; Linux just truncates the string.
		&& strlen(str) <511
#endif
		)
	{
		Assign(stackBuff);
		return;
	}
	char *buff=0, *newBuff;
	size_t buffSize=8096;
	while (1)
	{
		newBuff = (char*) rakRealloc_Ex(buff, buffSize,__FILE__,__LINE__);
		if (newBuff==0)
		{
			notifyOutOfMemory(__FILE__, __LINE__);
			if (buff!=0)
			{
				Assign(buff);
				rakFree_Ex(buff,__FILE__,__LINE__);
			}
			else
			{
				Assign(stackBuff);
			}
			return;
		}
		buff=newBuff;
		if (_vsnprintf(buff, buffSize, str, ap)!=-1)
		{
			Assign(buff);
			rakFree_Ex(buff,__FILE__,__LINE__);
			return;
		}
		buffSize*=2;
	}
}
RakNet::RakString RakString::Assign(const char *str,size_t pos, size_t n )
{
	size_t incomingLen=strlen(str);

	Clone();

	if (str==0 || str[0]==0||pos>=incomingLen)
	{
		sharedString=&emptyString;
		return (*this);
	}

	if (pos+n>=incomingLen)
	{
	n=incomingLen-pos;
	
	}
	const char * tmpStr=&(str[pos]); 

	size_t len = n+1;
	Allocate(len);
	memcpy(sharedString->c_str, tmpStr, len);
	sharedString->c_str[n]=0;

	return (*this);
}

RakNet::RakString RakString::NonVariadic(const char *str)
{
	RakNet::RakString rs;
	rs=str;
	return rs;
}
unsigned long RakString::ToInteger(const char *str)
{
	unsigned long hash = 0;
	int c;

	while (c = *str++)
		hash = c + (hash << 6) + (hash << 16) - hash;

	return hash;
}
unsigned long RakString::ToInteger(const RakString &rs)
{
	return RakString::ToInteger(rs.C_String());
}
void RakString::AppendBytes(const char *bytes, unsigned int count)
{
	Clone();
	Realloc(sharedString, count);
	unsigned int length=(unsigned int) GetLength();
	memcpy(sharedString->c_str+length, bytes, count);
	sharedString->c_str[length+count]=0;
}
void RakString::Clone(void)
{
	if (sharedString==&emptyString)
	{
		return;
	}

	// Empty or solo then no point to cloning
	sharedString->refCountMutex->Lock();
	if (sharedString->refCount==1)
	{
		sharedString->refCountMutex->Unlock();
		return;
	}

	sharedString->refCount--;
	sharedString->refCountMutex->Unlock();
	Assign(sharedString->c_str);
}
void RakString::Free(void)
{
	if (sharedString==&emptyString)
		return;
	sharedString->refCountMutex->Lock();
	sharedString->refCount--;
	if (sharedString->refCount==0)
	{
		sharedString->refCountMutex->Unlock();
		const size_t smallStringSize = 128-sizeof(unsigned int)-sizeof(size_t)-sizeof(char*)*2;
		if (sharedString->bytesUsed>smallStringSize)
			rakFree_Ex(sharedString->bigString, __FILE__, __LINE__ );
		/*
		poolMutex->Lock();
		pool.Release(sharedString);
		poolMutex->Unlock();
		*/

		RakString::LockMutex();
		RakString::freeList.Insert(sharedString, __FILE__, __LINE__);
		RakString::UnlockMutex();

		sharedString=&emptyString;
	}
	else
	{
		sharedString->refCountMutex->Unlock();
	}
	sharedString=&emptyString;
}
unsigned char RakString::ToLower(unsigned char c)
{
	if (c >= 'A' && c <= 'Z')
		return c-'A'+'a';
	return c;
}
unsigned char RakString::ToUpper(unsigned char c)
{
	if (c >= 'a' && c <= 'z')
		return c-'a'+'A';
	return c;
}
void RakString::LockMutex(void)
{
	GetPoolMutex().Lock();
}
void RakString::UnlockMutex(void)
{
	GetPoolMutex().Unlock();
}

/*
#include "RakString.h"
#include <string>
#include "GetTime.h"

using namespace RakNet;

int main(void)
{
	RakString s3("Hello world");
	RakString s5=s3;

	RakString s1;
	RakString s2('a');

	RakString s4("%i %f", 5, 6.0);

	RakString s6=s3;
	RakString s7=s6;
	RakString s8=s6;
	RakString s9;
	s9=s9;
	RakString s10(s3);
	RakString s11=s10 + s4 + s9 + s2;
	s11+=RakString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
	RakString s12("Test");
	s12+=s11;
	bool b1 = s12==s12;
	s11=s5;
	s12.ToUpper();
	s12.ToLower();
	RakString s13;
	bool b3 = s13.IsEmpty();
	s13.Set("blah %s", s12.C_String());
	bool b4 = s13.IsEmpty();
	size_t i1=s13.GetLength();
	s3.Clear(__FILE__, __LINE__);
	s4.Clear(__FILE__, __LINE__);
	s5.Clear(__FILE__, __LINE__);
	s5.Clear(__FILE__, __LINE__);
	s6.Printf();
	s7.Printf();
	RAKNET_DEBUG_PRINTF("\n");

	static const int repeatCount=750;
	DataStructures::List<RakString> rakStringList;
	DataStructures::List<std::string> stdStringList;
	DataStructures::List<char*> referenceStringList;
	char *c;
	unsigned i;
	RakNetTime beforeReferenceList, beforeRakString, beforeStdString, afterStdString;

	unsigned loop;
	for (loop=0; loop<2; loop++)
	{
		beforeReferenceList=RakNet::GetTime();
		for (i=0; i < repeatCount; i++)
		{
			c = RakNet::OP_NEW_ARRAY<char >(56,__FILE__, __LINE__ );
			strcpy(c, "Aalsdkj alsdjf laksdjf ;lasdfj ;lasjfd");
			referenceStringList.Insert(c);
		}
		beforeRakString=RakNet::GetTime();
		for (i=0; i < repeatCount; i++)
			rakStringList.Insert("Aalsdkj alsdjf laksdjf ;lasdfj ;lasjfd");
		beforeStdString=RakNet::GetTime();

		for (i=0; i < repeatCount; i++)
			stdStringList.Insert("Aalsdkj alsdjf laksdjf ;lasdfj ;lasjfd");
		afterStdString=RakNet::GetTime();
		RAKNET_DEBUG_PRINTF("Insertion 1 Ref=%i Rak=%i, Std=%i\n", beforeRakString-beforeReferenceList, beforeStdString-beforeRakString, afterStdString-beforeStdString);

		beforeReferenceList=RakNet::GetTime();
		for (i=0; i < repeatCount; i++)
		{
			RakNet::OP_DELETE_ARRAY(referenceStringList[0], __FILE__, __LINE__);
			referenceStringList.RemoveAtIndex(0);
		}
		beforeRakString=RakNet::GetTime();
		for (i=0; i < repeatCount; i++)
			rakStringList.RemoveAtIndex(0);
		beforeStdString=RakNet::GetTime();
		for (i=0; i < repeatCount; i++)
			stdStringList.RemoveAtIndex(0);
		afterStdString=RakNet::GetTime();
		RAKNET_DEBUG_PRINTF("RemoveHead Ref=%i Rak=%i, Std=%i\n", beforeRakString-beforeReferenceList, beforeStdString-beforeRakString, afterStdString-beforeStdString);

		beforeReferenceList=RakNet::GetTime();
		for (i=0; i < repeatCount; i++)
		{
			c = RakNet::OP_NEW_ARRAY<char >(56, __FILE__, __LINE__ );
			strcpy(c, "Aalsdkj alsdjf laksdjf ;lasdfj ;lasjfd");
			referenceStringList.Insert(0);
		}
		beforeRakString=RakNet::GetTime();
		for (i=0; i < repeatCount; i++)
			rakStringList.Insert("Aalsdkj alsdjf laksdjf ;lasdfj ;lasjfd");
		beforeStdString=RakNet::GetTime();
		for (i=0; i < repeatCount; i++)
			stdStringList.Insert("Aalsdkj alsdjf laksdjf ;lasdfj ;lasjfd");
		afterStdString=RakNet::GetTime();
		RAKNET_DEBUG_PRINTF("Insertion 2 Ref=%i Rak=%i, Std=%i\n", beforeRakString-beforeReferenceList, beforeStdString-beforeRakString, afterStdString-beforeStdString);

		beforeReferenceList=RakNet::GetTime();
		for (i=0; i < repeatCount; i++)
		{
			RakNet::OP_DELETE_ARRAY(referenceStringList[referenceStringList.Size()-1], __FILE__, __LINE__);
			referenceStringList.RemoveAtIndex(referenceStringList.Size()-1);
		}
		beforeRakString=RakNet::GetTime();
		for (i=0; i < repeatCount; i++)
			rakStringList.RemoveAtIndex(rakStringList.Size()-1);
		beforeStdString=RakNet::GetTime();
		for (i=0; i < repeatCount; i++)
			stdStringList.RemoveAtIndex(stdStringList.Size()-1);
		afterStdString=RakNet::GetTime();
		RAKNET_DEBUG_PRINTF("RemoveTail Ref=%i Rak=%i, Std=%i\n", beforeRakString-beforeReferenceList, beforeStdString-beforeRakString, afterStdString-beforeStdString);

	}

	printf("Done.");
	char str[128];
	gets(str);
	return 1;
}
*/
