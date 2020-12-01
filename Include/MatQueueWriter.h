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
 */#pragma once

#include <queue>
#include <string>
#include <mutex>

#pragma push_macro("check")
#undef check
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/types_c.h"
#pragma pop_macro("check")

struct MatQueueContainer
{
	std::string folderPath;
	cv::Mat mat;
};

class MatQueueWriter
{
private:
	std::queue<MatQueueContainer> matQueue;
	std::mutex queueMutex;

public:
	MatQueueWriter() {}
	MatQueueWriter(MatQueueWriter const&) = delete;
	void operator=(MatQueueWriter const&) = delete;

	__declspec(dllexport) void QueueMat(
		const std::string & outputPath,
		cv::Mat inputMat,
		const bool debug);

	__declspec(dllexport) void Poll(const bool debug);

private:
	// void ValidateFilePath(std::string & folderPath, std::string& fileName, std::string& outputPath);
};

extern "C" __declspec(dllexport) MatQueueWriter & GetMatQueueWriter();