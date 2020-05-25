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
	static OpenCVWrapper & Get ()
	{
		static OpenCVWrapper instance;
		return instance;
	}

private:
	OpenCVWrapper() {}

public:
	OpenCVWrapper(OpenCVWrapper const&) = delete;
	void operator=(OpenCVWrapper const&) = delete;

	bool FindChessboardCorners(
		cv::Mat image,
		cv::Size patternSize,
		float * outputArray,
		// std::vector<cv::Point2f> imageCorners,
		int findFlags
	);
};