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

#include "converter/ConverterManager.h"
#include "util/StringUtility.h"
#include "converter/LogWriter.h"

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;
const std::wstring F4DCONVERTER_VERSION = L"1.0.0";

void print_version()
{
	std::wcout << "F4DConverter " << F4DCONVERTER_VERSION.c_str() << std::endl;
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

std::map<std::wstring, std::wstring> arguments;

// arguments for conversion configuration
std::wstring inputFolder(L"Input Folder");
std::wstring outputFolder(L"Output Folder");
std::wstring logPath(L"Log File Path");
std::wstring indexCreation(L"Create Indices");
std::wstring idPrefix(L"idPrefix");
std::wstring idSuffix(L"idSuffix");
// arguments for processing parameters
std::wstring occlusionCulling(L"Occlusion Culling");
std::wstring unitScaleFactor(L"Unit Scale Factor");

bool extractArguments(int argc, char** argv)
{
	std::wstring token;
	std::vector<std::wstring> tokens;

	for (int i = 1; i < argc; i++)
	{
		token = gaia3d::s2ws(argv[i]);
		tokens.push_back(token);
	}

	size_t tokenCount = tokens.size();
	for (size_t i = 0; i < tokenCount; i++)
	{
		std::wcout << i << L" : " << tokens[i] << std::endl;
		if (tokens[i].substr(0, 1) == std::wstring(L"-"))
		{
			if (i == tokenCount - 1 || tokens[i + 1].substr(0, 1) == std::wstring(L"-"))
			{
				return false;
			}

			if (tokens[i] == std::wstring(L"-inputFolder"))
			{
				arguments[inputFolder] = tokens[i + 1];
				i++;
				continue;
			}
			if (tokens[i] == std::wstring(L"-outputFolder"))
			{
				arguments[outputFolder] = tokens[i + 1];
				i++;
				continue;
			}
			if (tokens[i] == std::wstring(L"-log"))
			{
				arguments[logPath] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::wstring(L"-indexing"))
			{
				arguments[indexCreation] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::wstring(L"-idPrefix"))
			{
				arguments[idPrefix] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::wstring(L"-idSuffix"))
			{
				arguments[idSuffix] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::wstring(L"-oc"))
			{
				arguments[occlusionCulling] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::wstring(L"-usf"))
			{
				arguments[unitScaleFactor] = tokens[i + 1];
				i++;
				continue;
			}

			// TODO(khj 20170305) : LOG invalid arguments
			return false;
		}
		else
		{
			// TODO(khj 20170305) : LOG invalid arguments
			return false;
		}
	}

	if (arguments.find(outputFolder) == arguments.end())
		return false;

	if (arguments.find(inputFolder) == arguments.end() && arguments.find(indexCreation) == arguments.end())
		return false;

	if (arguments.find(inputFolder) != arguments.end())
	{
		if (arguments.find(logPath) == arguments.end())
			return false;
	}

	if (arguments.find(indexCreation) != arguments.end())
	{
		if (arguments[indexCreation] != std::wstring(L"Y") &&
			arguments[indexCreation] != std::wstring(L"y") &&
			arguments[indexCreation] != std::wstring(L"N") &&
			arguments[indexCreation] != std::wstring(L"n"))
			return false;
	}

	if (arguments.find(occlusionCulling) != arguments.end())
	{
		if (arguments[occlusionCulling] != std::wstring(L"Y") &&
			arguments[occlusionCulling] != std::wstring(L"y") &&
			arguments[occlusionCulling] != std::wstring(L"N") &&
			arguments[occlusionCulling] != std::wstring(L"n"))
			return false;
	}

	if (arguments.find(unitScaleFactor) != arguments.end())
	{
		try
		{
			double scaleFactor = std::stod(arguments[unitScaleFactor]);

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
			return false;
	}

	if (arguments.find(logPath) != arguments.end())
	{
		LogWriter::getLogWriter()->start();
		LogWriter::getLogWriter()->setFullPath(arguments[logPath]);
	}

	print_version();

	if (isValidParams)
	{
		ConverterManager::getConverterManager()->setIsCliMode(true);
		if (arguments.find(inputFolder) != arguments.end())
		{
			ConverterManager::getConverterManager()->setConversionOn(true);
			ConverterManager::getConverterManager()->setInputFolder(arguments[inputFolder]);
			ConverterManager::getConverterManager()->setOutputFolder(arguments[outputFolder]);

			if (arguments.find(occlusionCulling) != arguments.end())
			{
				if (arguments[occlusionCulling] == std::wstring(L"Y") ||
					arguments[occlusionCulling] == std::wstring(L"y"))
					ConverterManager::getConverterManager()->setOcclusionCulling(true);
				else
					ConverterManager::getConverterManager()->setOcclusionCulling(false);
			}
			else
				ConverterManager::getConverterManager()->setOcclusionCulling(false);

			if (arguments.find(unitScaleFactor) != arguments.end())
			{
				ConverterManager::getConverterManager()->setUnitScaleFactor(std::stod(arguments[unitScaleFactor]));
			}
		}
		else
		{
			ConverterManager::getConverterManager()->setConversionOn(false);
		}

		if (arguments.find(indexCreation) != arguments.end())
		{
			if (arguments[indexCreation] == std::wstring(L"Y") ||
				arguments[indexCreation] == std::wstring(L"y"))
			{
				ConverterManager::getConverterManager()->setIndexCreation(true);
				ConverterManager::getConverterManager()->setOutputFolder(arguments[outputFolder]);
			}
			else
			{
				ConverterManager::getConverterManager()->setIndexCreation(false);
			}
		}
		else
		{
			ConverterManager::getConverterManager()->setIndexCreation(false);
		}


		if (arguments.find(idPrefix) != arguments.end())
		{
			ConverterManager::getConverterManager()->setIdPrefix(arguments[idPrefix]);
		}

		if (arguments.find(idSuffix) != arguments.end())
		{
			ConverterManager::getConverterManager()->setIdSuffix(arguments[idSuffix]);
		}
	}
	else
	{
		ConverterManager::getConverterManager()->setIsCliMode(false);
	}

	std::cout << "Starting GLFW context, OpenGL 3.3" << std::endl;

	// Init GLFW
	glfwInit();

	// Set all the required options for GLFW
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	//glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

#if __APPLE__
	// uncomment this statement to fix compilation on OS X
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "F4DConverter", NULL, NULL);
	
	if (ConverterManager::getConverterManager()->getIsCliMode())
	{
		if (ConverterManager::getConverterManager()->getConversionOn())
		{
			if (!ConverterManager::getConverterManager()->isInitialized())
			{
				if (!ConverterManager::getConverterManager()->initialize(window, WIDTH, HEIGHT))
				{
					LogWriter::getLogWriter()->finish();
					LogWriter::getLogWriter()->save();
					return false;
				}

				ConverterManager::getConverterManager()->processDataFolder();

				LogWriter::getLogWriter()->finish();
				LogWriter::getLogWriter()->save();
			}
		}

		if (ConverterManager::getConverterManager()->getIndexCreation())
			ConverterManager::getConverterManager()->writeIndexFile();

		//::AfxGetMainWnd()->PostMessage(WM_CLOSE);
	}
	else
	{
		if (ConverterManager::getConverterManager()->isInitialized())
		{
			ConverterManager::getConverterManager()->changeGLDimension(WIDTH, HEIGHT);
		}
		else
		{
			ConverterManager::getConverterManager()->initialize(window, WIDTH, HEIGHT);
		}
	}
	/*
	// RenderLoop
	while (!glfwWindowShouldClose(window))
	{
		float ratio;
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float)height;
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef((float)glfwGetTime() * 50.f, 0.f, 0.f, 1.f);
		glBegin(GL_TRIANGLES);
		glColor3f(1.f, 0.f, 0.f);
		glVertex3f(-0.6f, -0.4f, 0.f);
		glColor3f(0.f, 1.f, 0.f);
		glVertex3f(0.6f, -0.4f, 0.f);
		glColor3f(0.f, 0.f, 1.f);
		glVertex3f(0.f, 0.6f, 0.f);
		glEnd();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	*/

	glfwDestroyWindow(window);

	// Terminates GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();

	return 0;
}