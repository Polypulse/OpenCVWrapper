#pragma once

#include <vector>

#pragma push_macro("check")
#undef check
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/types_c.h"
#pragma pop_macro("check")

class OpenCVWrapper
{
public:
	OpenCVWrapper() {}
	OpenCVWrapper(OpenCVWrapper const&) = delete;
	void operator=(OpenCVWrapper const&) = delete;

	__declspec(dllexport) bool FindChessboardCorners(
		cv::Mat image,
		cv::Size patternSize,
		// float * outputArray,
		std::vector<cv::Point2f> imageCorners,
		int findFlags
	);
};

extern "C" __declspec(dllexport) OpenCVWrapper & GetOpenCVWrapper()
{
	static OpenCVWrapper instance;
	return instance;
}