/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "MatQueueWriter.h"
#include <cstdlib>
// #include <filesystem>
// #include <vector>
#include "windows.h"
#include "WrapperLogQueue.h"

extern "C" __declspec(dllexport) MatQueueWriter & GetMatQueueWriter()
{
	static MatQueueWriter instance;
	return instance;
}

extern "C" __declspec(dllexport) void MatQueueWriter::QueueMat(
	const std::string & outputPath,
	cv::Mat inputMat,
	const bool debug)
{
	MatQueueContainer container;
	container.folderPath = outputPath;
	container.mat = inputMat;

	queueMutex.lock();
	matQueue.push(container);
	queueMutex.unlock();
}

extern "C" __declspec(dllexport) void MatQueueWriter::Poll(const bool debug)
{
	bool isQueued = matQueue.empty() == false;
	while (isQueued)
	{
		MatQueueContainer container;
		container = matQueue.front();
		matQueue.pop();

		std::string outputPath = container.folderPath/* + "/corner-visualization.jpg"*/;
		cv::Mat image = container.mat;

		if (image.empty())
		{
			GetWrapperLogQueue().QueueLog("Unable to write image to file: \"" + outputPath + "\", image is empty.", 2);
			return;
		}

		if (!cv::imwrite(outputPath, image))
		{
			GetWrapperLogQueue().QueueLog("Unable to write image to file: \"" + outputPath + "\".", 2);
			return;
		}

		if (debug)
			GetWrapperLogQueue().QueueLog("Successfully wrote image to file: \"" + outputPath + "\".", 0);

		isQueued = matQueue.empty() == false;
	}
}

/*
void MatQueueWriter::ValidateFilePath(std::string & folderPath, std::string& fileName, std::string& outputPath)
{
	std::string targetExtension = ".jpg";
	std::filesystem::path fileP(fileName);

	std::string parentPath;
	bool directoryExisted = true;

	if (fileP.has_extension())
	{
		targetExtension = fileP.extension().string();
		parentPath = fileP.parent_path().string();
		if (std::filesystem::exists(parentPath))
		{
			std::filesystem::create_directories(parentPath);
			directoryExisted = false;
		}
	}

	else if (std::filesystem::exists(folderPath))
	{
		std::filesystem::create_directories(folderPath);
		parentPath = folderPath;
		directoryExisted = false;
	}

	std::string fileNameWithoutExtension = fileP.stem().string();
	std::string outputFilename(fileNameWithoutExtension);

	if (directoryExisted)
	{
		std::vector<std::string> files;
		for (const auto& p : std::filesystem::directory_iterator(folderPath))
		{
			if (p.path().stem().string().find(fileNameWithoutExtension) != std::string::npos && p.path().extension() == targetExtension)
				files.push_back(p.path().stem().string());
		}

		std::sort(files.begin(), files.end());
		std::string lastFileName = files[files.size() - 1];
		int lastIndexOfDash = lastFileName.find_last_of("-") + 1;
		std::string indexStr = lastFileName.substr(lastIndexOfDash, lastFileName.find_first_of(".") - lastIndexOfDash);
		int index = std::stoi(indexStr);
		index++;
		outputFilename += "-" + std::to_string(index) + targetExtension;
	}

	outputPath = parentPath + "/" + outputFilename;
}
*/
