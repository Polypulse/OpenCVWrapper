#include "WrapperLogQueue.h"

extern "C" __declspec(dllexport) WrapperLogQueue & GetWrapperLogQueue ()
{
	static WrapperLogQueue instance;
	return instance;
}

extern "C" __declspec(dllexport) bool WrapperLogQueue::LogIsQueued()
{
	bool isQueued;
	queueMutex.lock();
	isQueued = logQueue.size() > 0;
	queueMutex.unlock();
	return isQueued;
}

extern "C" __declspec(dllexport) int WrapperLogQueue::PeekNextSize()
{
	int size;
	queueMutex.lock();
	const WrapperLogContainer * logContainer = &logQueue.front();
	size = sizeof(char) * static_cast<int>(logContainer->message.size());
	queueMutex.unlock();
	return size;
}

extern "C" __declspec(dllexport) int WrapperLogQueue::DequeueLog(char* buffer, int & messageType)
{
	queueMutex.lock();
	const WrapperLogContainer logContainer = logQueue.front();
	logQueue.pop();
	queueMutex.unlock();

	memcpy(buffer, logContainer.message.c_str(), sizeof(char) * logContainer.message.size());
	messageType = logContainer.messageType;

	return static_cast<int>(logContainer.message.size());
}

extern "C" __declspec(dllexport) void WrapperLogQueue::QueueLog(std::string message, int messageType)
{
	WrapperLogContainer logContainer;
	logContainer.message = message;
	logContainer.messageType = messageType;

	queueMutex.lock();
	logQueue.push(logContainer);
	queueMutex.unlock();
}
