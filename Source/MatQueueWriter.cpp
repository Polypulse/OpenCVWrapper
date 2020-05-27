/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "MatQueueWriter.h"
#include <cstdlib>
#include <filesystem>
#include <vector>

extern "C" __declspec(dllexport) MatQueueWriter & GetMatQueueWriter()
{
	static MatQueueWriter instance;
	return instance;
}

extern "C" __declspec(dllexport) void MatQueueWriter::QueueMat(std::string outputPath, cv::Mat inputMat)
{
	MatQueueContainer container;
	container.outputPath = outputPath;
	container.mat = inputMat;

	matQueue.push(container);
}

extern "C" __declspec(dllexport) void MatQueueWriter::Poll()
{
	bool isQueued = matQueue.empty() == false;
	while (isQueued)
	{
		MatQueueContainer container;
		container = matQueue.front();
		matQueue.pop();

		std::string folderPath = container.outputPath;
		std::string fileName = container.fileName;

		std::string outputPath;
		ValidateFilePath(folderPath, fileName, outputPath);

	/*
	if (filePath.IsEmpty() || !FPaths::ValidatePath(filePath))
	{
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*absoluteBackupFolderPath);
		filePath = FPaths::Combine(absoluteBackupFolderPath, backupFileName);
		filePath = LensSolverUtilities::GenerateIndexedFilePath(filePath, backupExtension);
		return true;
	}

	FString folder = FPaths::GetPath(filePath);
	if (folder.IsEmpty())
		folder = absoluteBackupFolderPath;

	if (FPaths::IsRelative(filePath))
		filePath = FPaths::ConvertRelativePathToFull(filePath);

	if (FPaths::IsDrive(filePath))
	{
		filePath = FPaths::Combine(filePath, backupFileName);
		filePath = LensSolverUtilities::GenerateIndexedFilePath(filePath, backupExtension);
		return true;
	}

	FString fileName = FPaths::GetBaseFilename(filePath);
	FString extension = FPaths::GetExtension(filePath);

	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*folder);

	if (extension.IsEmpty())
	{
		if (FPaths::FileExists(folder))
		{
			UE_LOG(LogTemp, Error, TEXT("Cannot create path to file: \"%s\", the folder: \"%s\" is a file."), *filePath, *folder);
			return false;
		}

		filePath = FPaths::Combine(folder, backupFileName);
		filePath = LensSolverUtilities::GenerateIndexedFilePath(filePath, backupExtension);
		return true;
	}

	filePath = FPaths::Combine(folder, fileName);
	filePath = LensSolverUtilities::GenerateIndexedFilePath(filePath, extension);
	return true;
	*/


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

		if (!cv::imwrite(outputPath, container.mat))
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

