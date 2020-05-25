#include "OpenCVWrapper.h"

extern "C" __declspec(dllexport) bool OpenCVWrapper::FindChessboardCorners(
	cv::Mat image,
	cv::Size patternSize,
	// float * outputArray,
	std::vector<cv::Point2f> imageCorners,
	int findFlags)
{
	/*
	std::vector<cv::Point2f> imageCorners;
	bool patternFound = cv::findChessboardCorners(image, patternSize, imageCorners, findFlags);
	if (!patternFound)
		return false;

	for (int i = 0; i < imageCorners.size(); i++)
	{
		*(outputArray + (i + 0) * 2) = imageCorners[i].x;
		*(outputArray + (i + 1) * 2) = imageCorners[i].x;
	}
	*/

	return cv::findChessboardCorners(image, patternSize, imageCorners, findFlags);
}
