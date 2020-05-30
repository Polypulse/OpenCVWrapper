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

	float inverseResizeRatio = resizeParameters.nativeX / (float)resizeParameters.resizeX;

	for (int i = 0; i < imageCount; i++)
	{
		for (int ci = 0; ci < cornerCount; ci++)
		{
			corners[i][ci].x = *(cornersData + (i * cornerCount) * 2 + ci * 2)/* * inverseResizeRatio*/;
			corners[i][ci].y = *(cornersData + (i * cornerCount) * 2 + ci * 2 + 1)/* * inverseResizeRatio*/;

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
	cv::Mat cameraMatrix = cv::Mat::eye(3, 3, cv::DataType<double>::type);
	cv::Mat distortionCoefficients = cv::Mat::zeros(8, 1, cv::DataType<double>::type);
	cv::Size imageSize(resizeParameters.resizeX, resizeParameters.resizeY);
	cv::Point2d principalPoint = cv::Point2d(0.0, 0.0);
	cv::TermCriteria termCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001f);

	int sourcePixelWidth = resizeParameters.nativeX;
	int sourcePixelHeight = resizeParameters.nativeY;
	
	float sensorHeight = calibrationParameters.sensorDiagonalSizeMM / std::sqrt(std::powf(sourcePixelWidth / (float)sourcePixelHeight, 2.0f) + 1.0f);
	float sensorWidth = sensorHeight * (sourcePixelWidth / (float)sourcePixelHeight);

	if (debug)
		GetWrapperLogQueue().QueueLog("Sensor size: (" + std::to_string(sensorWidth) + ", " + std::to_string(sensorHeight) + ") mm, diagonal: (" + std::to_string(calibrationParameters.sensorDiagonalSizeMM) + " mm.", 0);

	double fovX = 0.0f, fovY = 0.0f, focalLength = 0.0f;
	double aspectRatio = 0.0f;

	int flags = 0;
	flags |= calibrationParameters.useInitialIntrinsicValues			?	cv::CALIB_USE_INTRINSIC_GUESS : 0;
	flags |= calibrationParameters.keepPrincipalPixelPositionFixed		?	cv::CALIB_FIX_PRINCIPAL_POINT : 0;
	flags |= calibrationParameters.keepAspectRatioFixed					?	cv::CALIB_FIX_ASPECT_RATIO : 0;
	flags |= calibrationParameters.lensHasTangentalDistortion			?	cv::CALIB_ZERO_TANGENT_DIST : 0;
	flags |= calibrationParameters.fixRadialDistortionCoefficientK1		?	cv::CALIB_FIX_K1 : 0;
	flags |= calibrationParameters.fixRadialDistortionCoefficientK2		?	cv::CALIB_FIX_K2 : 0;
	flags |= calibrationParameters.fixRadialDistortionCoefficientK3		?	cv::CALIB_FIX_K3 : 0;
	flags |= calibrationParameters.fixRadialDistortionCoefficientK4		?	cv::CALIB_FIX_K4 : 0;
	flags |= calibrationParameters.fixRadialDistortionCoefficientK5		?	cv::CALIB_FIX_K5 : 0;
	flags |= calibrationParameters.fixRadialDistortionCoefficientK6		?	cv::CALIB_FIX_K6 : 0;

	if (flags & cv::CALIB_USE_INTRINSIC_GUESS)
	{
		cameraMatrix.at<double>(0, 2) = static_cast<double>(calibrationParameters.initialPrincipalPointNativePixelPositionX);
		cameraMatrix.at<double>(1, 2) = static_cast<double>(calibrationParameters.initialPrincipalPointNativePixelPositionY);
		if (debug)
			GetWrapperLogQueue().QueueLog("Setting initial principal point to: (" + 
				std::to_string(calibrationParameters.initialPrincipalPointNativePixelPositionX) + ", " +
				std::to_string(calibrationParameters.initialPrincipalPointNativePixelPositionY) + ")", 0);
	}

	else if (flags & cv::CALIB_FIX_ASPECT_RATIO)
	{
		// cameraMatrix.at<double>(0, 0) = 1.0 / (resizeParameters.nativeX * 0.5);
		// cameraMatrix.at<double>(1, 1) = 1.0 / (resizeParameters.nativeY * 0.5);
		cameraMatrix.at<double>(0, 0) = (resizeParameters.nativeX / static_cast<double>(resizeParameters.nativeY));
		/*
		if (debug)
			GetWrapperLogQueue().QueueLog("Keeping aspect ratio at: " + std::to_string(resizeParameters.nativeX / (double)resizeParameters.nativeY), 0);
		*/
	}

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

	fovX *= inverseResizeRatio;
	fovY *= inverseResizeRatio;
	focalLength *= inverseResizeRatio;

	principalPoint.x = sourcePixelWidth * (principalPoint.x / sensorWidth);
	principalPoint.y = sourcePixelHeight * (principalPoint.y / sensorHeight);

	output.fovX = (float)fovX;
	output.fovY = (float)fovY;
	output.focalLengthMM = (float)focalLength;
	output.aspectRatio = (float)aspectRatio;
	output.sensorSizeMMX = (float)sensorWidth;
	output.sensorSizeMMY = (float)sensorHeight;
	output.principalPixelPointX = (float)principalPoint.x;
	output.principalPixelPointY = (float)principalPoint.y;
	output.resolutionX = resizeParameters.nativeX;
	output.resolutionY = resizeParameters.nativeY;
	output.k1 = (float)distortionCoefficients.at<double>(0, 0);
	output.k2 = (float)distortionCoefficients.at<double>(1, 0);
	output.p1 = (float)distortionCoefficients.at<double>(2, 0);
	output.p2 = (float)distortionCoefficients.at<double>(3, 0);
	output.k3 = (float)distortionCoefficients.at<double>(4, 0);


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

	image = cv::Mat(width, height, cv::DataType<uint8_t>::type);

	int pixelCount = width * height;
	for (int pi = 0; pi < pixelCount; pi++)
		image.at<uint8_t>(pi / width, pi % height) = *(pixels + pi);

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

	int findFlags = cv::CALIB_CB_NORMALIZE_IMAGE;
	findFlags |= cv::CALIB_CB_ADAPTIVE_THRESH;

	if (textureSearchParameters.exhaustiveSearch)
		findFlags |= cv::CALIB_CB_EXHAUSTIVE;

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

	if (textureSearchParameters.writeDebugTextureToFile)
	{
		cv::drawChessboardCorners(image, patternSize, imageCorners, patternFound);
		WriteMatToFile(image, textureSearchParameters.debugTextureOutputPath, debug);
	}

	// data = reinterpret_cast<float*>(imageCorners.data());
	memcpy(data, imageCorners.data(), sizeof(float) * patternSize.width * patternSize.height * 2);
	return true;
}
