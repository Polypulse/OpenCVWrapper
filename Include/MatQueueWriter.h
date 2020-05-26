/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include <queue>

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
	std::string outputPath;
	cv::Mat mat;
};

class MatQueueWriter
{
public:
	static MatQueueWriter & Get()
	{
		static MatQueueWriter instance;
		return instance;
	}


private:
	std::queue<MatQueueContainer> matQueue;
	MatQueueWriter() {}

	std::wstring defaultOutputPath;

public:
	MatQueueWriter(MatQueueWriter const&) = delete;
	void operator=(MatQueueWriter const&) = delete;

	__declspec(dllexport) void QueueMat(std::string outputPath, cv::Mat inputMat);
	__declspec(dllexport) void Poll();

	__declspec(dllexport) void SetDefaultOutputPath (std::wstring newDefaultOutputPath);
};