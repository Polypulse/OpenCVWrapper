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
#include "OpenCVWrapper.h"
#include "MatQueueWriter.h"
#include "WrapperLogQueue.h"

#include <cmath>
#include <random>

extern "C" __declspec(dllexport) OpenCVWrapper & GetOpenCVWrapper()
{
	static OpenCVWrapper instance;
	return instance;
}

extern "C" __declspec(dllexport) bool OpenCVWrapper::ProcessImageFromFile (
	FResizeParameters & resizeParameters,
	const FChessboardSearchParameters & textureSearchParameters,
	const char absoluteFilePath[260], 
	float * data,
	const bool debug)
{

	int sourceWidth, sourceHeight;
	cv::Mat image;
	std::string filePath(absoluteFilePath);
	if (!GetImageFromFile(filePath, image, sourceWidth, sourceHeight, debug))
	{
		// TODO
		return false;
	}

	resizeParameters.sourceX = sourceWidth;
	resizeParameters.sourceY = sourceHeight;
	if (textureSearchParameters.resize)
	{
		resizeParameters.resizeX = (int)(sourceWidth * textureSearchParameters.resizePercentage);
		resizeParameters.resizeY = (int)(sourceHeight * textureSearchParameters.resizePercentage);
	}

	else
	{
		resizeParameters.resizeX = sourceWidth;
		resizeParameters.resizeY = sourceHeight;
	}

	return ProcessImage(resizeParameters, textureSearchParameters, image, data, debug);
}

extern "C" __declspec(dllexport) bool OpenCVWrapper::ProcessImageFromPixels (
	const FResizeParameters & resizeParameters,
	const FChessboardSearchParameters & textureSearchParameters,
	const uint8_t * pixels,
	const int stride,
	const int width,
	const int height, 
	float * data,
	const bool debug)
{
	cv::Mat image;
	if (!GetImageFromArray(pixels, stride, width, height, image, debug))
	{
		// TODO
		return false;
	}

	return ProcessImage(resizeParameters, textureSearchParameters, image, data, debug);
}

extern "C" __declspec(dllexport) bool OpenCVWrapper::CalibrateLens(
	const FResizeParameters & resizeParameters,
	const FCalibrateLensParameters & calibrationParameters,
	const float * cornersData,
	const float chessboardSquareSizeMM,
	const int cornerCountX,
	const int cornerCountY,
	const int imageCount,
	FCalibrateLensOutput & output,
	const bool debug)
{
	const int cornerCount = cornerCountX * cornerCountY;
	std::vector<std::vector<cv::Point2f>> corners(imageCount, std::vector<cv::Point2f>(cornerCount));
	std::vector<std::vector<cv::Point3f>> objectPoints(imageCount, std::vector<cv::Point3f>(cornerCount));

	double inverseResizeRatio = resizeParameters.nativeX / (double)resizeParameters.resizeX;

	for (int i = 0; i < imageCount; i++)
	{
		for (int ci = 0; ci < cornerCount; ci++)
		{
			corners[i][ci].x = *(cornersData + (i * cornerCount) * 2 + ci * 2) * (float)inverseResizeRatio;
			corners[i][ci].y = *(cornersData + (i * cornerCount) * 2 + ci * 2 + 1) * (float)inverseResizeRatio;

			objectPoints[i][ci] = cv::Point3d(
				chessboardSquareSizeMM * (ci % cornerCountX), 
				chessboardSquareSizeMM * (ci / cornerCountX), 0.0);
		}
	}

	// std::random_device rd;
    // std::mt19937 g(rd());

	// std::shuffle(std::begin(corners), std::end(corners), g);

	if (debug)
		GetWrapperLogQueue().QueueLog("Done dequeuing work units, preparing calibration using " + std::to_string(corners.size()) + " sets of points.", 0);

	std::vector<cv::Mat> rvecs, tvecs;

	// cv::Size imageSize(resizeParameters.resizeX, resizeParameters.resizeY);
	cv::Size imageSize(resizeParameters.nativeX, resizeParameters.nativeY);
	cv::TermCriteria termCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 60, 0.1f);

	cv::Mat cameraMatrix				= cv::Mat::eye(3, 3, cv::DataType<double>::type);
	cv::Mat distortionCoefficients;
	if (!calibrationParameters.useRationalModel)
		distortionCoefficients = cv::Mat::zeros(8, 1, cv::DataType<double>::type);
	else distortionCoefficients = cv::Mat::zeros(5, 1, cv::DataType<double>::type);
	cv::Point2d principalPoint			= cv::Point2d(resizeParameters.resizeX * 0.5, resizeParameters.resizeY * 0.5);

	int sourcePixelWidth = resizeParameters.nativeX;
	int sourcePixelHeight = resizeParameters.nativeY;
	
	float sensorHeight = calibrationParameters.sensorDiagonalSizeMM / std::sqrt(std::powf(sourcePixelWidth / (float)sourcePixelHeight, 2.0f) + 1.0f);
	float sensorWidth = sensorHeight * (sourcePixelWidth / (float)sourcePixelHeight);

	if (debug)
		GetWrapperLogQueue().QueueLog("Sensor size: (" + std::to_string(sensorWidth) + ", " + std::to_string(sensorHeight) + ") mm, diagonal: (" + std::to_string(calibrationParameters.sensorDiagonalSizeMM) + " mm.", 0);

	double fovX = 0.0, fovY = 0.0, focalLength = 0.0;
	double aspectRatio = 0.0;

	int flags = 0;
	flags |= calibrationParameters.useInitialIntrinsicValues			?	cv::CALIB_USE_INTRINSIC_GUESS : 0;
	flags |= calibrationParameters.keepPrincipalPixelPositionFixed		?	cv::CALIB_FIX_PRINCIPAL_POINT : 0;
	flags |= calibrationParameters.keepAspectRatioFixed					?	cv::CALIB_FIX_ASPECT_RATIO : 0;
	flags |= calibrationParameters.lensHasTangentalDistortion == false	?	cv::CALIB_ZERO_TANGENT_DIST : 0;
	flags |= calibrationParameters.fixRadialDistortionCoefficientK1		?	cv::CALIB_FIX_K1 : 0;
	flags |= calibrationParameters.fixRadialDistortionCoefficientK2		?	cv::CALIB_FIX_K2 : 0;
	flags |= calibrationParameters.fixRadialDistortionCoefficientK3		?	cv::CALIB_FIX_K3 : 0;

	if (calibrationParameters.useRationalModel)
	{
		flags |= cv::CALIB_RATIONAL_MODEL;
		flags |= calibrationParameters.fixRadialDistortionCoefficientK4		?	cv::CALIB_FIX_K4 : 0;
		flags |= calibrationParameters.fixRadialDistortionCoefficientK5		?	cv::CALIB_FIX_K5 : 0;
		flags |= calibrationParameters.fixRadialDistortionCoefficientK6		?	cv::CALIB_FIX_K6 : 0;
	}

	if (flags & cv::CALIB_USE_INTRINSIC_GUESS || flags & cv::CALIB_FIX_PRINCIPAL_POINT)
	{
		double px = static_cast<double>(calibrationParameters.initialPrincipalPointNativePixelPositionX);
		double py = static_cast<double>(calibrationParameters.initialPrincipalPointNativePixelPositionY);

		cameraMatrix.at<double>(0, 2) = px;
		cameraMatrix.at<double>(1, 2) = py;

		principalPoint.x = px;
		principalPoint.y = py;

		if (debug)
			GetWrapperLogQueue().QueueLog("Setting initial principal point to: (" + 
				std::to_string(calibrationParameters.initialPrincipalPointNativePixelPositionX) + ", " +
				std::to_string(calibrationParameters.initialPrincipalPointNativePixelPositionY) + ")", 0);
	}

	double aspect = resizeParameters.nativeY / static_cast<double>(resizeParameters.nativeX);
	cameraMatrix.at<double>(0, 0) = aspect;

	/*
	else if (flags & cv::CALIB_FIX_ASPECT_RATIO)
	{
		if (debug)
			GetWrapperLogQueue().QueueLog("Fixing aspect ratio at: " + std::to_string(aspect), 0);
	}
	*/

	GetWrapperLogQueue().QueueLog("Calibrating...", 0);

	output.error = (float)cv::calibrateCamera(
		objectPoints,
		corners,
		imageSize,
		cameraMatrix,
		distortionCoefficients,
		rvecs,
		tvecs,
		flags,
		termCriteria);

	GetWrapperLogQueue().QueueLog("Finished calibration.", 0);

	cv::calibrationMatrixValues(cameraMatrix, imageSize, sensorWidth, sensorHeight, fovX, fovY, focalLength, principalPoint, aspectRatio);

	principalPoint.x = sourcePixelWidth * (principalPoint.x / sensorWidth);
	principalPoint.y = sourcePixelHeight * (principalPoint.y / sensorHeight);

	output.fovX					= static_cast<float>(fovX);
	output.fovY					= static_cast<float>(fovY);
	output.focalLengthMM		= static_cast<float>(focalLength);
	output.aspectRatio			= static_cast<float>(aspectRatio);
	output.sensorSizeMMX		= static_cast<float>(sensorWidth);
	output.sensorSizeMMY		= static_cast<float>(sensorHeight);
	output.principalPixelPointX = static_cast<float>(principalPoint.x);
	output.principalPixelPointY = static_cast<float>(principalPoint.y);

	output.resolutionX			= resizeParameters.nativeX;
	output.resolutionY			= resizeParameters.nativeY;

	output.k1					= static_cast<float>(distortionCoefficients.at<double>(0, 0));
	output.k2					= static_cast<float>(distortionCoefficients.at<double>(1, 0));
	output.p1					= static_cast<float>(distortionCoefficients.at<double>(2, 0));
	output.p2					= static_cast<float>(distortionCoefficients.at<double>(3, 0));
	output.k3					= static_cast<float>(distortionCoefficients.at<double>(4, 0));

	if (calibrationParameters.useRationalModel)
	{
		output.k4 = static_cast<float>(distortionCoefficients.at<double>(5, 0));
		output.k5 = static_cast<float>(distortionCoefficients.at<double>(6, 0));
		output.k6 = static_cast<float>(distortionCoefficients.at<double>(7, 0));
	}

	return true;
}

bool OpenCVWrapper::GetImageFromArray(
	const uint8_t * pixels,
	int stride,
	int width,
	int height,
	cv::Mat& image,
	const bool debug)
{
	if (debug)
		GetWrapperLogQueue().QueueLog("Copying pixel data of pixel count: " + std::to_string(width * height) + " to image of size: (" + std::to_string(width) + ", " + std::to_string(height) + ").", 0);

	image = cv::Mat(height, width, cv::DataType<uint8_t>::type);

	int pixelCount = width * height;
	for (int pi = 0; pi < pixelCount; pi++)
		image.at<uint8_t>(pi / width, pi % width) = *(pixels + pi * sizeof(uint));

	if (debug)
		GetWrapperLogQueue().QueueLog("Done copying pixel data.", 0);

	return true;
}

bool OpenCVWrapper::GetImageFromFile(
	const std::string & absoluteFilePath,
	cv::Mat& image,
	int & sourceWidth,
	int & sourceHeight,
	const bool debug)
{
	std::string str(absoluteFilePath);
	std::replace(str.begin(), str.end(), '\\', '/');

	if (debug)
		GetWrapperLogQueue().QueueLog("Attempting to read image from path: \"" + str + "\".", 0);

	image = cv::imread(str);

	if (image.data == NULL)
	{
		GetWrapperLogQueue().QueueLog("Unable texture from path: \"" + str + "\".", 2);
		return false;
	}

	cv::cvtColor(image, image, cv::COLOR_BGR2GRAY);
	sourceWidth = image.cols;
	sourceHeight = image.rows;

	if (debug)
		GetWrapperLogQueue().QueueLog("Loaded texture from path: \"" + str + "\" at resolution: (" + std::to_string(sourceWidth) + ", " + std::to_string(sourceHeight) + ").", 0);

	return true;
}

void OpenCVWrapper::WriteMatToFile(
	cv::Mat image,
	const std::string & outputPath,
	const bool debug)
{
	GetMatQueueWriter().QueueMat(outputPath, image, debug);

	if (debug)
		GetWrapperLogQueue().QueueLog("Queued image to be written to path: \"" + outputPath + "\".", 0);
}

bool OpenCVWrapper::ProcessImage(
	const FResizeParameters & resizeParameters,
	const FChessboardSearchParameters & textureSearchParameters,
	cv::Mat image,
	float * data,
	const bool debug)
{
	float resizePercentage = textureSearchParameters.resizePercentage;
	float checkerBoardSquareSizeMM = textureSearchParameters.checkerBoardSquareSizeMM;

	if (debug)
		GetWrapperLogQueue().QueueLog("Prepared image of size: (" + std::to_string(image.cols) + ", " + std::to_string(image.rows) + ").", 0);

	cv::Size sourceImageSize(resizeParameters.sourceX, resizeParameters.sourceY);
	cv::Size resizedImageSize(resizeParameters.resizeX, resizeParameters.resizeY);

	float inverseResizeRatio = resizeParameters.nativeX / (float)resizeParameters.nativeY;

	if (textureSearchParameters.resize && textureSearchParameters.resizePercentage != 1.0f)
	{
		cv::resize(image, image, resizedImageSize, 0.0f, 0.0f, cv::INTER_LINEAR);

		if (debug)
			GetWrapperLogQueue().QueueLog("Resized image from: (" + 
				std::to_string(resizeParameters.sourceX) + ", " +
				std::to_string(resizeParameters.sourceY) + ") to: (" +
				std::to_string(resizeParameters.resizeX) + ", " +
				std::to_string(resizeParameters.resizeY) + ").", 0);
	}

	cv::TermCriteria termCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001f);
	std::vector<cv::Point2f> imageCorners;

	cv::Size patternSize(
		textureSearchParameters.checkerBoardCornerCountX,
		textureSearchParameters.checkerBoardCornerCountY);

	int findFlags	= cv::CALIB_CB_NORMALIZE_IMAGE;
	findFlags		|= cv::CALIB_CB_ADAPTIVE_THRESH;

	if (textureSearchParameters.exhaustiveSearch)
		findFlags |= cv::CALIB_CB_EXHAUSTIVE;

	if (textureSearchParameters.writePreCornerDetectionTextureToFile)
		WriteMatToFile(image, textureSearchParameters.preCornerDetectionTextureOutputPath, debug);

	if (debug)
		GetWrapperLogQueue().QueueLog("Beginning calibration pattern detection for image.", 0);

	bool patternFound = cv::findChessboardCorners(image, patternSize, imageCorners, findFlags);

	if (!patternFound)
	{
		GetWrapperLogQueue().QueueLog("Found no pattern in image.", 1);
		return false;
	}

	if (debug)
		GetWrapperLogQueue().QueueLog("Found pattern in image.", 1);

	cv::TermCriteria cornerSubPixCriteria(
		cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS,
		50,
		0.0001
	);

	cv::cornerSubPix(image, imageCorners, cv::Size(5, 5), cv::Size(-1, -1), cornerSubPixCriteria);

	if (textureSearchParameters.writeCornerVisualizationTextureToFile)
	{
		cv::drawChessboardCorners(image, patternSize, imageCorners, patternFound);
		WriteMatToFile(image, textureSearchParameters.cornerVisualizationTextureOutputPath, debug);
	}

	// data = reinterpret_cast<float*>(imageCorners.data());
	memcpy(data, imageCorners.data(), sizeof(float) * patternSize.width * patternSize.height * 2);
	return true;
}
