#pragma once
#include <queue>
#include <mutex>

struct WrapperLogContainer
{
	int messageType; // 0 Info, 1 Warning, 2 Error.
	std::string message;
};

class WrapperLogQueue
{
private:
	std::queue<WrapperLogContainer> logQueue;
	std::mutex queueMutex;
public:
	WrapperLogQueue() {}
	WrapperLogQueue(WrapperLogQueue const&) = delete;
	void operator=(WrapperLogQueue const&) = delete;

	bool LogIsQueued ();
	int PeekNextSize ();
	int DequeueLog (char * buffer, int & messageType);
	void QueueLog (std::string message, int messageType);
};

extern "C" __declspec(dllexport) WrapperLogQueue & GetWrapperLogQueue ()
{
	static WrapperLogQueue instance;
	return instance;
}
