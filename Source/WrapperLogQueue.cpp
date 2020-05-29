#include "WrapperLogQueue.h"

bool WrapperLogQueue::LogIsQueued()
{
	bool isQueued;
	queueMutex.lock();
	isQueued = logQueue.size() > 0;
	queueMutex.unlock();
	return isQueued;
}

int WrapperLogQueue::PeekNextSize()
{
	const WrapperLogContainer * logContainer = &logQueue.front();
	return sizeof(char) * logContainer->message.size();
}

int WrapperLogQueue::DequeueLog(char* buffer, int & messageType)
{
	queueMutex.lock();
	const WrapperLogContainer logContainer = logQueue.front();
	logQueue.pop();
	queueMutex.unlock();

	memcpy(buffer, logContainer.message.c_str(), sizeof(char) * logContainer.message.size());

	return logContainer.message.size();
}

void WrapperLogQueue::QueueLog(std::string message, int messageType)
{
	WrapperLogContainer logContainer;
	logContainer.message = message;
	logContainer.messageType = messageType;

	queueMutex.lock();
	logQueue.push(logContainer);
	queueMutex.unlock();
}
