#ifdef _WIN32
#define APIENTRY __stdcall
#endif

#include <glad/glad.h>

// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
#error windows.h was included!
#endif

#include <GLFW/glfw3.h>

#include <iostream>
#include <map>
#include <vector>

#include "argdefinition.h"
#include "converter/ConverterManager.h"
#include "util/StringUtility.h"
#include "converter/LogWriter.h"

const std::string F4DCONVERTER_VERSION = "1.0.0";

void print_version()
{
	std::cout << "F4DConverter " << F4DCONVERTER_VERSION.c_str() << std::endl;
}

/*
-inputFolder [rawDataFolder] : an absolute path of the folder where raw data files to be converted are.
-outputFolder [F4DFolder] : an absolute path of the folder where conversion results(F4D datasets) are created.
-log [logFileFullPath] : an absolute path of a log file which is created after finishing conversion processes.
-idPrefix [prefix] : a prefix used in name of an F4D folder.
-idSuffix [suffix] : a suffix used in name of an F4D folder.
-oc [one of Y, y, N, n] : whether visibility indices for occlusion culling should be created or not. "NOT created" is default.
-usf [numericValue] : unit scale factor. Geometries in F4D are described in meter. That is, the unit scale factor of raw data descrived in centimeter is 0.01 for example.
-indexing [one of Y, y, N, n] : wheter objectIndexFile.ihe should be created or not. "NOT created" is default.
*/

std::map<std::string, std::string> arguments;

bool extractArguments(int argc, char** argv)
{
	std::string token;
	std::vector<std::string> tokens;

	for (int i = 1; i < argc; i++)
	{
		//token = gaia3d::s2ws(argv[i]);
		token = std::string(argv[i]);
		tokens.push_back(std::string(token));
	}

	size_t tokenCount = tokens.size();
	for (size_t i = 0; i < tokenCount; i++)
	{
		//std::cout << i << " : " << tokens[i] << std::endl;
		if (tokens[i].substr(0, 1) == std::string("-"))
		{
			if (i == tokenCount - 1 || tokens[i + 1].substr(0, 1) == std::string("-"))
			{
				return false;
			}

			if (tokens[i] == std::string(InputFolder))
			{
				arguments[InputFolder] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::string(OutputFolder))
			{
				arguments[OutputFolder] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::string(LogFilePath))
			{
				arguments[LogFilePath] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::string(CreateIndex))
			{
				arguments[CreateIndex] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::string(IdPrefix))
			{
				arguments[IdPrefix] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::string(IdSuffix))
			{
				arguments[IdSuffix] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::string(PerformOC))
			{
				arguments[PerformOC] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::string(UnitScaleFactor))
			{
				arguments[UnitScaleFactor] = tokens[i + 1];
				i++;
				continue;
			}
		}
		else
		{
			return false;
		}
	}

	if (arguments.find(OutputFolder) == arguments.end())
		return false;

	if (arguments.find(InputFolder) == arguments.end() && arguments.find(CreateIndex) == arguments.end())
		return false;

	if (arguments.find(InputFolder) != arguments.end())
	{
		if (arguments.find(LogFilePath) == arguments.end())
			return false;
	}

	if (arguments.find(CreateIndex) != arguments.end())
	{
		if (arguments[CreateIndex] != std::string("Y") &&
			arguments[CreateIndex] != std::string("y") &&
			arguments[CreateIndex] != std::string("N") &&
			arguments[CreateIndex] != std::string("n"))
			return false;
	}

	if (arguments.find(PerformOC) != arguments.end())
	{
		if (arguments[PerformOC] != std::string("Y") &&
			arguments[PerformOC] != std::string("y") &&
			arguments[PerformOC] != std::string("N") &&
			arguments[PerformOC] != std::string("n"))
			return false;
	}

	if (arguments.find(UnitScaleFactor) != arguments.end())
	{
		try
		{
			double scaleFactor = std::stod(arguments[UnitScaleFactor]);

			if (scaleFactor < 0.001)
				return false;
		}
		catch (const std::invalid_argument& error)
		{
			std::string errorMessage = error.what();
			return false;
		}
		catch (const std::out_of_range& error)
		{
			std::string errorMessage = error.what();
			return false;
		}
	}

	return true;
}

// The MAIN function, from here we start the application and run the render loop
int main(int argc, char** argv)
{
	bool isValidParams = false;
	if (argc != 0)
	{
		if (!(isValidParams = extractArguments(argc, argv)))
		{
			print_version();
			return false;
		}
	}

	// arguments log
	printf("[STEP]Arguments are following.\n");
	std::map<std::string, std::string>::iterator iter = arguments.begin();
	for (; iter != arguments.end(); iter++)
	{
		printf("%s : %s\n", iter->first.c_str(), iter->second.c_str());
	}

	// TODO(khj 20180424) : NYI must make log file through logger system
	// start log writer if needed
	if (arguments.find(LogFilePath) != arguments.end())
	{
		LogWriter::getLogWriter()->start();
		LogWriter::getLogWriter()->setFullPath(arguments[LogFilePath]);
	}

	print_version();

	// process
	ConverterManager::getConverterManager()->initialize();
	ConverterManager::getConverterManager()->setProcessConfiguration(arguments);
	ConverterManager::getConverterManager()->process();
	ConverterManager::getConverterManager()->uninitialize();

	// TODO(khj 20180424) : NYI must make log file through logger system
	// finish and save log if log writing started
	if (LogWriter::getLogWriter()->isStarted())
	{
		LogWriter::getLogWriter()->finish();
		LogWriter::getLogWriter()->save();
	}

	return 0;
}