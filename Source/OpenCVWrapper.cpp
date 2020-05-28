#include "OpenCVWrapper.h"
#include "MatQueueWriter.h"
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
	float *& data)
{

	int sourceWidth, sourceHeight;
	cv::Mat image;
	std::string filePath(absoluteFilePath);
	if (!GetImageFromFile(filePath, image, sourceWidth, sourceHeight))
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

	return ProcessImage(resizeParameters, textureSearchParameters, image, data);
}

extern "C" __declspec(dllexport) bool OpenCVWrapper::ProcessImageFromPixels (
	const FResizeParameters & resizeParameters,
	const FChessboardSearchParameters & textureSearchParameters,
	const uint8_t * pixels,
	const int stride,
	const int width,
	const int height, 
	float *& data)
{
	cv::Mat image;
	if (!GetImageFromArray(pixels, stride, width, height, image))
	{
		// TODO
		return false;
	}

	return ProcessImage(resizeParameters, textureSearchParameters, image, data);
}

extern "C" __declspec(dllexport) bool OpenCVWrapper::CalibrateLens(
	const FResizeParameters & resizeParameters,
	const FCalibrateLensParameters & calibrationParameters,
	const float * cornersData,
	const float chessboardSquareSizeMM,
	const int cornerCountX,
	const int cornerCountY,
	const int imageCount,
	FCalibrateLensOutput & output)
{
	const int cornerCount = cornerCountX * cornerCountY;
	std::vector<std::vector<cv::Point2f>> corners(imageCount, std::vector<cv::Point2f>(cornerCount));
	std::vector<std::vector<cv::Point3f>> objectPoints(imageCount, std::vector<cv::Point3f>(cornerCount));

	/*
	int i = 0;
	for (int y = 0; y < textureSearchParameters.checkerBoardCornerCountY; y++)
		for (int x = 0; x < textureSearchParameters.checkerBoardCornerCountX; x++)
			objectPoints[i++] = FVector(
				x * textureSearchParameters.checkerBoardSquareSizeMM,
				y * textureSearchParameters.checkerBoardSquareSizeMM,
				0.0f);
	*/

	for (int i = 0; i < imageCount; i++)
	{
		for (int ci = 0; ci < cornerCount; ci++)
		{
			corners[i][ci].x = *(cornersData + (i * cornerCount) + ci * 2);
			corners[i][ci].y = *(cornersData + (i * cornerCount) + ci * 2 + 1);

			objectPoints[i][ci] = cv::Point3d(
				(ci / (double)cornerCountX) * chessboardSquareSizeMM, 
				std::fmod((double)ci, (double)cornerCountY) * chessboardSquareSizeMM, 0.0);
		}
	}

	std::random_device rd;
    std::mt19937 g(rd());

	std::shuffle(std::begin(corners), std::end(corners), g);

	/*
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): Done dequeing work units, preparing calibration using %d sets of points."), corners.size()));
	*/

	std::vector<cv::Mat> rvecs, tvecs;
	cv::Mat cameraMatrix = cv::Mat::eye(3, 3, cv::DataType<double>::type);
	cv::Mat distortionCoefficients = cv::Mat::zeros(5, 1, cv::DataType<double>::type);
	cv::Size sourceImageSize(resizeParameters.nativeX, resizeParameters.nativeY);
	cv::Point2d principalPoint = cv::Point2d(0.0, 0.0);
	cv::TermCriteria termCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001f);

	int sourcePixelWidth = resizeParameters.nativeX;
	int sourcePixelHeight = resizeParameters.nativeY;
	
	float sensorHeight = calibrationParameters.sensorDiagonalSizeMM / std::sqrt(std::powf(sourcePixelWidth / (float)sourcePixelHeight, 2.0f) + 1.0f);
	float sensorWidth = sensorHeight * (sourcePixelWidth / (float)sourcePixelHeight);
	/*
	float sensorHeight = (latchData.calibrationParameters.sensorDiagonalSizeMM * sourcePixelWidth) / FMath::Sqrt(sourcePixelWidth * sourcePixelWidth + sourcePixelHeight * sourcePixelHeight);
	float sensorWidth = sensorHeight * (sourcePixelWidth / (float)sourcePixelHeight);
	*/

	/*
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): Sensor size: (%f, %f) mm, diagonal: (%f) mm."), sensorWidth, sensorHeight, latchData.calibrationParameters.sensorDiagonalSizeMM));
	*/

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
		/*
		if (Debug())
			QueueLog(FString::Printf(TEXT("(INFO): Setting initial principal point to: (%f, %f)"), 
				calibrationParameters.initialPrincipalPointNativePixelPositionX,
				calibrationParameters.initialPrincipalPointNativePixelPositionY));
		*/
	}

	else if (flags & cv::CALIB_FIX_ASPECT_RATIO)
	{
		cameraMatrix.at<double>(0, 0) = 1.0 / (resizeParameters.nativeX * 0.5);
		cameraMatrix.at<double>(1, 1) = 1.0 / (resizeParameters.nativeY * 0.5);
		/*
		if (Debug())
			QueueLog(FString::Printf(TEXT("(INFO): Keeping aspect ratio at: %f"), 
				(resizeParameters.nativeX / (double)resizeParameters.nativeY)));
		*/
	}

	// QueueLog("(INFO): Calibrating...");

	output.error = (float)cv::calibrateCamera(
		objectPoints,
		corners,
		sourceImageSize,
		cameraMatrix,
		distortionCoefficients,
		rvecs,
		tvecs,
		flags,
		termCriteria);
	/*
	try
	{
		error = cv::calibrateCamera(
			objectPoints,
			corners,
			sourceImageSize,
			cameraMatrix,
			distortionCoefficients,
			rvecs,
			tvecs,
			flags,
			termCriteria);
	}

	// catch (const cv::Exception& exception)
	catch (...)
	{
		// FString exceptionMsg = UTF8_TO_TCHAR(exception.msg.c_str());
		// QueueLog(FString::Printf(TEXT("(ERROR): OpenCV exception: \"%s\"."), *exceptionMsg));
		QueueLog("(ERROR): OpenCV exception occurred.");
		QueueCalibrationResultError(latchData.baseParameters);
		return;
	}
	*/

	/*
	if (ShouldExit())
		return;

	QueueLog("(INFO): Done.");
	*/

	cv::calibrationMatrixValues(cameraMatrix, sourceImageSize, sensorWidth, sensorHeight, fovX, fovY, focalLength, principalPoint, aspectRatio);
	// FMatrix perspectiveMatrix = GeneratePerspectiveMatrixFromFocalLength(sourceImageSize, principalPoint, focalLength);

	fovX *= 2.0f;
	fovY *= 2.0f;
	focalLength *= 2.0f;

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
	cv::Mat& image)
{
	/*
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): Copying pixel data of pixel count: %d to image of size: (%d, %d)."), pixels.Num(), resolution.X, resolution.Y));
	*/

	image = cv::Mat(width, height, cv::DataType<uint8_t>::type);

	int pixelCount = width * height;
	for (int pi = 0; pi < pixelCount; pi++)
		image.at<uint8_t>(pi / width, pi % height) = *(pixels + pi);

	/*
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): Done copying pixel data."), pixels.Num(), resolution.X, resolution.Y));
	*/

	return true;
}

bool OpenCVWrapper::GetImageFromFile(
	const std::string & absoluteFilePath,
	cv::Mat& image,
	int & sourceWidth,
	int & sourceHeight)
{
	std::string str(absoluteFilePath);
	std::replace(str.begin(), str.end(), '\\', '/');

	/*
	if (Debug())
		QueueLog(FString::Printf(TEXT("Attempting to read image from path: \"%s\""), *absoluteFilePath));
	*/

	image = cv::imread(str);

	if (image.data == NULL)
	{
		/*
		if (Debug())
			QueueLog(FString::Printf(TEXT("(ERROR) Unable texture from path: \"%s\""), *absoluteFilePath));
		*/
		return false;
	}

	cv::cvtColor(image, image, cv::COLOR_BGR2GRAY);
	sourceWidth = image.cols;
	sourceHeight = image.rows;

	/*
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): Loaded texture from path: \"%s\" at resolution: (%d, %d)."), *absoluteFilePath, sourceResolution.X, sourceResolution.Y));
	*/
	return true;
}

void OpenCVWrapper::WriteMatToFile(
	cv::Mat image,
	const std::string & outputPath)
{
	GetMatQueueWriter().QueueMat(outputPath, image);
	/*
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): Queued texture: \'%s\" to be written to file."), *outputPath));
	*/
}

bool OpenCVWrapper::ProcessImage(
	const FResizeParameters & resizeParameters,
	const FChessboardSearchParameters & textureSearchParameters,
	cv::Mat image,
	float *& data)
{
	float resizePercentage = textureSearchParameters.resizePercentage;
	bool resize = textureSearchParameters.resize;

	float checkerBoardSquareSizeMM = textureSearchParameters.checkerBoardSquareSizeMM;
	// resizeParameters.resizeResolution = resizeParameters.sourceResolution * resizePercentage;

	// QueueLog(FString::Printf(TEXT("%sPrepared image of size: (%d, %d!"), *workerMessage, image.cols, image.rows));

	/*
	if (resize)
	{
		resizeParameters.resizeResolution.X = FMath::FloorToInt(resizeParameters.sourceResolution.X * resizePercentage);
		resizeParameters.resizeResolution.Y = FMath::FloorToInt(resizeParameters.sourceResolution.Y * resizePercentage);
	}

	else
	{
		resizeParameters.resizeResolution.X = resizeParameters.sourceResolution.X;
		resizeParameters.resizeResolution.Y = resizeParameters.sourceResolution.Y;
	}
	*/

	cv::Size sourceImageSize(resizeParameters.sourceX, resizeParameters.sourceY);
	cv::Size resizedImageSize(resizeParameters.resizeX, resizeParameters.resizeY);

	float inverseResizeRatio = resizeParameters.nativeX / (float)resizeParameters.nativeY;

	if (resize && resizePercentage != 1.0f)
	{
		/*
		if (Debug())
			QueueLog(FString::Printf(TEXT("(INFO): %s: Resizing image from: (%d, %d) to: (%d, %d)."),
				*JobDataToString(baseParameters),
				resizeParameters.sourceX,
				resizeParameters.sourceY,
				resizeParameters.resizeX,
				resizeParameters.resizeY));
		*/

		cv::resize(image, image, resizedImageSize, 0.0f, 0.0f, cv::INTER_LINEAR);
	}

	/*/
	if (image.rows != resizedPixelWidth || image.cols != resizedPixelHeight)
	{
		UE_LOG(LogTemp, Log, TEXT("%sAllocating image from size: (%d, %d) to: (%d, %d)."), *workerMessage, image.cols, image.rows, resizedPixelWidth, resizedPixelHeight);
		image = cv::Mat(resizedPixelHeight, resizedPixelWidth, cv::DataType<uint8>::type);
	}
	*/

	/*
	UE_LOG(LogTemp, Log, TEXT("%sResized pixel size: (%d, %d), source size: (%d, %d), resize ratio: %f."),
		*workerMessage,
		resizedPixelWidth,
		resizedPixelHeight,
		resizeParameters.sourceResolution.X,
		resizeParameters.sourceResolution.Y,
		1.0f / inverseResizeRatio);
	*/

	cv::TermCriteria termCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001f);

	std::vector<cv::Point2f> imageCorners;

	cv::Size patternSize(
		textureSearchParameters.checkerBoardCornerCountX,
		textureSearchParameters.checkerBoardCornerCountY);

	int findFlags = cv::CALIB_CB_NORMALIZE_IMAGE;
	findFlags |= cv::CALIB_CB_ADAPTIVE_THRESH;

	if (textureSearchParameters.exhaustiveSearch)
		findFlags |= cv::CALIB_CB_EXHAUSTIVE;

	/*
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Beginning calibration pattern detection for image: \"%s\"."), *JobDataToString(baseParameters), *baseParameters.friendlyName));
	*/

	bool patternFound = cv::findChessboardCorners(image, patternSize, imageCorners, findFlags);

	if (!patternFound)
	{
		// QueueLog(FString::Printf(TEXT("(INFO): %s: Found no pattern in image: \"%s\", queuing empty work unit."), *JobDataToString(baseParameters), *baseParameters.friendlyName));
		// QueueEmptyCalibrationPointsWorkUnit(baseParameters, resizeParameters);
		return false;
	}

	/*
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Found calibration pattern in image: \"%s\"."), *JobDataToString(baseParameters), *baseParameters.friendlyName));
	*/

	cv::TermCriteria cornerSubPixCriteria(
		cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS,
		50,
		0.0001
	);

	cv::cornerSubPix(image, imageCorners, cv::Size(5, 5), cv::Size(-1, -1), cornerSubPixCriteria);
	/*
	try
	{
		cv::cornerSubPix(image, imageCorners, cv::Size(5, 5), cv::Size(-1, -1), cornerSubPixCriteria);
	}

	// catch (const cv::Exception& exception)
	catch (...)
	{
		// FString exceptionMsg = UTF8_TO_TCHAR(exception.msg.c_str());
		// QueueLog(FString::Printf(TEXT("(ERROR): OpenCV exception: \"%s\"."), *exceptionMsg));
		QueueLog("(ERROR): OpenCV exception occurred.");
		QueueEmptyCalibrationPointsWorkUnit(baseParameters, resizeParameters);
		return;
	}
	*/

	if (textureSearchParameters.writeDebugTextureToFile)
	{
		// cv::drawChessboardCorners(image, patternSize, imageCorners, patternFound);
		WriteMatToFile(image, textureSearchParameters.debugTextureOutputPath);
	}

	data = reinterpret_cast<float*>(imageCorners.data());

	return true;
}
