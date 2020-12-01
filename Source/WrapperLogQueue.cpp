/*
 * Copyright (C) 2020 - LensCalibrator contributors, see Contributors.txt at the root of the project.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
