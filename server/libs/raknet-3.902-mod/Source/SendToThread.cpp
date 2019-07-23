#include "SendToThread.h"
#ifdef USE_THREADED_SEND
#include "RakThread.h"
#include "InternalPacket.h"
#include "GetTime.h"

#if USE_SLIDING_WINDOW_CONGESTION_CONTROL!=1
#include "CCRakNetUDT.h"
#else
#include "CCRakNetSlidingWindow.h"
#endif

int SendToThread::refCount=0;
DataStructures::ThreadsafeAllocatingQueue<SendToThread::SendToThreadBlock> SendToThread::objectQueue;
ThreadPool<SendToThread::SendToThreadBlock*,SendToThread::SendToThreadBlock*> SendToThread::threadPool;

SendToThread::SendToThreadBlock* SendToWorkerThread(SendToThread::SendToThreadBlock* input, bool *returnOutput, void* perThreadData)
{
	(void) perThreadData;
	*returnOutput=false;
	RakNetTimeUS *mostRecentTime=(RakNetTimeUS *)input->data;
	*mostRecentTime=RakNet::GetTimeUS();
	SocketLayer::SendTo(input->s, input->data, input->dataWriteOffset, input->binaryAddress, input->port, input->remotePortRakNetWasStartedOn_PS3);
	SendToThread::objectQueue.Push(input);
	return 0;
}
SendToThread::SendToThread()
{
}
SendToThread::~SendToThread()
{

}
void SendToThread::AddRef(void)
{
	if (++refCount==1)
	{
		threadPool.StartThreads(1,0);
	}
}
void SendToThread::Deref(void)
{
	if (refCount>0)
	{
		if (--refCount==0)
		{
			threadPool.StopThreads();
			RakAssert(threadPool.NumThreadsWorking()==0);

			unsigned i;
			SendToThreadBlock* info;
			for (i=0; i < threadPool.InputSize(); i++)
			{
				info = threadPool.GetInputAtIndex(i);
				objectQueue.Push(info);
			}
			threadPool.ClearInput();
			objectQueue.Clear(__FILE__, __LINE__);
		}
	}
}
SendToThread::SendToThreadBlock* SendToThread::AllocateBlock(void)
{
	return objectQueue.PopOrAllocate();
}
void SendToThread::ProcessBlock(SendToThread::SendToThreadBlock* threadedSend)
{
	RakAssert(threadedSend->dataWriteOffset>0 && threadedSend->dataWriteOffset<=MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
	threadPool.AddInput(SendToWorkerThread,threadedSend);
}
#endif
