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

	__declspec(dllexport) bool LogIsQueued ();
	__declspec(dllexport) int PeekNextSize ();
	__declspec(dllexport) int DequeueLog (char * buffer, int & messageType);
	__declspec(dllexport) void QueueLog (std::string message, int messageType);
};

extern "C" __declspec(dllexport) WrapperLogQueue & GetWrapperLogQueue ();
