#include "OpenCVWrapper.h"
#include "MatQueueWriter.h"

extern "C" __declspec(dllexport) OpenCVWrapper & GetOpenCVWrapper()
{
	static OpenCVWrapper instance;
	return instance;
}

extern "C" __declspec(dllexport) bool OpenCVWrapper::ProcessImageFromFile (
	FResizeParameters & resizeParameters,
	const FChessboardSearchParameters & textureSearchParameters,
	const std::string absoluteFilePath, 
	float *& data)
{

	int sourceWidth, sourceHeight;
	cv::Mat image;
	if (!GetImageFromFile(absoluteFilePath, image, sourceWidth, sourceHeight))
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

void OpenCVWrapper::WriteMatToFile(cv::Mat image, std::string outputPath)
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

	// cv::cornerSubPix(image, imageCorners, cv::Size(5, 5), cv::Size(-1, -1), cornerSubPixCriteria);
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

	return true;
}
