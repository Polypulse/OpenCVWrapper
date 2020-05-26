/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "MatQueueWriter.h"

extern "C" __declspec(dllexport) void MatQueueWriter::QueueMat(std::string outputPath, cv::Mat inputMat)
{
	MatQueueContainer container;
	container.outputPath = outputPath;
	container.mat = inputMat;

	matQueue.push(container);
}

extern "C" __declspec(dllexport) void MatQueueWriter::Poll()
{
	static std::string defaultFileName = "DebugImage";
	static std::string defaultExtension = "jpg";

	bool isQueued = matQueue.empty() == false;
	while (isQueued)
	{
		MatQueueContainer container;
		container = matQueue.front();
		matQueue.pop();

		std::string filePath = container.outputPath;
		/*
		if (!LensSolverUtilities::ValidateFilePath(filePath, defaultMatFolder, defaultFileName, defaultExtension))
		{
			UE_LOG(LogTemp, Error, TEXT("Unable to write image to file: \"%s\", cannot validate path."), *filePath);
			return;
		}
		*/

		if (container.mat.empty())
		{
			// UE_LOG(LogTemp, Error, TEXT("Unable to write image to file: \"%s\", image is empty."), *filePath);
			return;
		}

		if (!cv::imwrite(filePath, container.mat))
		{
			// UE_LOG(LogTemp, Error, TEXT("Unable to write image to file: \"%s\"."), *filePath);
			return;
		}

		// UE_LOG(LogTemp, Log, TEXT("Wrote image to file: \"%s\"."), *filePath);

		isQueued = matQueue.empty() == false;
	}
}

extern "C" __declspec(dllexport) void MatQueueWriter::SetDefaultOutputPath(std::wstring newDefaultOutputPath)
{
	defaultOutputPath = newDefaultOutputPath;
}
