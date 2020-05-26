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

struct FResizeParameters
{
	int sourceX, sourceY;
	int resizeX, resizeY;
	int nativeX, nativeY;

	FResizeParameters()
	{
		sourceX = 0; sourceY = 0;
		resizeX = 0;  resizeY = 0;
		nativeX = 0;  nativeY = 0;
	}
};

struct FChessboardSearchParameters
{
	int nativeFullResolutionX, nativeFullResolutionY;
	float resizePercentage;
	bool resize;
	bool flipX, flipY;
	bool exhaustiveSearch;
	float checkerBoardSquareSizeMM;
	int checkerBoardCornerCountX, checkerBoardCornerCountY;
	bool writeDebugTextureToFile;
	std::string debugTextureOutputPath;

	FChessboardSearchParameters()
	{
		nativeFullResolutionX = 0; nativeFullResolutionY = 0;
		resizePercentage = 0.5f;
		resize = true;
		bool flipX = false; flipY = false;
		exhaustiveSearch = false;
		checkerBoardSquareSizeMM = 12.7f;
		checkerBoardCornerCountX = 0; checkerBoardCornerCountY = 0;
		writeDebugTextureToFile = false;
		debugTextureOutputPath = "";
	}
};

class OpenCVWrapper
{
public:
	OpenCVWrapper() {}
	OpenCVWrapper(OpenCVWrapper const&) = delete;
	void operator=(OpenCVWrapper const&) = delete;

	__declspec(dllexport) bool ProcessImageFromFile (
		const FResizeParameters & resizeParameters,
		const FChessboardSearchParameters & textureSearchParameters,
		const std::string absoluteFilePath, 
		float *& data);

	__declspec(dllexport) bool ProcessImageFromPixels (
		const FResizeParameters & resizeParameters,
		const FChessboardSearchParameters & textureSearchParameters,
		const uint8_t * pixels,
		const int stride,
		const int width,
		const int height, 
		float *& data);

private:
	bool GetImageFromArray(
		const uint8_t * pixels,
		const int stride,
		const int width,
		const int height,
		cv::Mat& image);

	bool GetImageFromFile(
		const std::string & absoluteFilePath,
		cv::Mat& image,
		int & sourceWidth,
		int & sourceHeight);

	void WriteMatToFile(
		cv::Mat image,
		std::string outputPath);

	bool ProcessImage(
		const FResizeParameters & resizeParameters,
		const FChessboardSearchParameters & textureSearchParameters,
		cv::Mat image,
		float *& data);
};

extern "C" __declspec(dllexport) OpenCVWrapper & GetOpenCVWrapper();