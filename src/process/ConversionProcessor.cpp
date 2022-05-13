/**
 * Implementation of the ConversionProcessor class
 */
#include <iostream>
#include <algorithm>

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_STATIC
#include "stb_image_resize.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "predefinition.h"

#include "ConversionProcessor.h"
#include "SceneControlVariables.h"
#include "NetSurfaceMeshMaker.h"

#include "../converter/LogWriter.h"
#include "../util/GeometryUtility.h"
#include "../util/StringUtility.h"
#include "../util/Image2DUtils.h"
#include "../util/Rectangle.h"
#include "../util/Image2DSplitData.h"
#include "../util/Image2D.h"
#include "../geometry/Quadtree.h"
#include "../geometry/LegoBlock.h"

 // Function prototypes
static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	//std::cout << key << std::endl;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

ConversionProcessor::ConversionProcessor()
	:thisSpatialOctree(NULL)
{
	scv = new SceneControlVariables();

	longitude = latitude = 0.0;
	altitude = 0.0f;

	bResponsibleDisposingGeometries = true;

	// Texture Filp Y
	textureFlip[0] = false;
	textureFlip[1] = false;

	thumbnail = NULL;
	thumbnailWidth = 0;
	thumbnailHeight = 0;
}

ConversionProcessor::~ConversionProcessor()
{
	clear();
	uninitialize();

	delete scv;
}

bool ConversionProcessor::initialize()
{
	float ratio = (float)WindowWidth / WindowHeight;

	if (scv->m_window == NULL)
	{
		std::cout << "Starting GLFW context with OpenGL" << std::endl;

		// Init GLFW
		glfwInit();

		// Set all the required options for GLFW
		//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
		//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

#if __APPLE__
		// uncomment this statement to fix compilation on OS X
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

		// Create a GLFWwindow object that we can use for GLFW's functions
		scv->m_window = glfwCreateWindow(WindowWidth, WindowHeight, WindowTitle, NULL, NULL);
		if (scv->m_window == NULL)
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::string(CANNOT_INITIALIZE_WND), true);
			//replaced
			LogWriter::getLogWriter()->setStatus(false, std::string("ConversionProcessor::initialize : ") + std::string(CANNOT_INITIALIZE_WND));

			glfwTerminate();
			return false;
		}
	}
	//scv->m_window = window;
	scv->m_width = (int)WindowWidth;
	scv->m_height = (int)WindowHeight;

	// projection mode
	scv->tp_projection = PROJECTION_PERSPECTIVE;

	// // Init vars lumm openGL.***
	scv->ambientLight[0] = 0.2f; scv->ambientLight[1] = 0.2f; scv->ambientLight[2] = 0.2f; scv->ambientLight[3] = 1.0f;
	scv->diffuseLight[0] = 0.7f; scv->diffuseLight[1] = 0.7f; scv->diffuseLight[2] = 0.7f; scv->diffuseLight[3] = 1.0f;
	scv->specular[0] = 0.3f; scv->specular[1] = 0.3f; scv->specular[2] = 0.3f; scv->specular[3] = 1.0f;
	scv->lightPos[0] = -1000000.0f; scv->lightPos[1] = -1000000.0f; scv->lightPos[2] = 2000000.0f; scv->lightPos[3] = 1.0f; // Hem de ferlo fixe.***
	scv->specref[0] = 0.7f; scv->specref[1] = 0.7f; scv->specref[2] = 0.7f; scv->specref[3] = 1.0f;
	scv->ClearColor[0] = 1.0f; scv->ClearColor[1] = 1.0f; scv->ClearColor[2] = 1.0f; scv->ClearColor[3] = 1.0f;

	glfwMakeContextCurrent(scv->m_window);
	if (scv->m_window == NULL)
	{
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(CANNOT_CONNECT_GLC_TO_DC), true);
		// replaced
		LogWriter::getLogWriter()->setStatus(false, std::string("ConversionProcessor::initialize : ") + std::string(CANNOT_CONNECT_GLC_TO_DC));

		return false;
	}

	// Set the required callback functions
	glfwSetErrorCallback(error_callback);
	glfwSetKeyCallback(scv->m_window, key_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(CANNOT_CREATE_GL_CONTEXT), true);
		// replaced
		LogWriter::getLogWriter()->setStatus(false, std::string("ConversionProcessor::initialize : ") + std::string(CANNOT_CREATE_GL_CONTEXT));

		return false;
	}

	glfwSwapInterval(1);

	std::cout << "OpenGL" << std::string((const char*)glGetString(GL_VERSION)) << std::endl;
	std::cout << "Supported GLSL version is " << std::string((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION)) << std::endl;

	//// initialize gl
	glEnable(GL_TEXTURE_2D);
	// Enables Depth Testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearDepth(1.0f);

	// set buffer draw/read mode
	glReadBuffer(GL_BACK);
	glDrawBuffer(GL_BACK);

	// Reset the current projection matrix
	scv->m_perspective_far = 10000.0;
	glViewport(0, 0, scv->m_width, scv->m_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//gluPerspective(scv->m_perspective_angle,(GLfloat)scv->m_width/(GLfloat)scv->m_height, scv->m_perspective_near, scv->m_perspective_far);
	glm::mat4 projection = glm::perspective(glm::radians(scv->m_perspective_angle),
		(double)ratio,
		scv->m_perspective_near,
		scv->m_perspective_far);
	glLoadMatrixf((const GLfloat*)&projection[0][0]);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glShadeModel(GL_SMOOTH);
	glPolygonMode(GL_FRONT, GL_FILL);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	//glEnable(GL_POLYGON_SMOOTH);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0, 1.0);

	/*
	glPolygonOffset(0.0, 0.0);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	//glEnable(GL_POLYGON_SMOOTH);
	*/
	//InitGL();
	//End InitGL------------------------------------------------------------------------------------------------------------------------------------------------
	// SetUp lighting.******************************************************************************************************************************************
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);

	// Setup and enable light_0.***
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, scv->ambientLight);
	//glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);// Test.***
	glLightfv(GL_LIGHT0, GL_AMBIENT, scv->ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, scv->diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, scv->specular);
	glLightfv(GL_LIGHT0, GL_POSITION, scv->lightPos);
	glEnable(GL_LIGHT0);

	// Enable color tracking.***
	glEnable(GL_COLOR_MATERIAL);

	// Set material properties to follow glColor values.***
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	// A partir d'aqui, tots els materials tenen reflectivitat especular total amb brillo.***
	glMaterialfv(GL_FRONT, GL_SPECULAR, scv->specref);
	glMateriali(GL_FRONT, GL_SHININESS, 64);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	//SetUpLighting();// Increiblement "SetUpLighting()" ha d'anar a darrera de "InitGL()".***
	// End SetUp lighting.---------------------------------------------------------------------------------------------------------------------------------

	return true;
}

void ConversionProcessor::uninitialize()
{
	if (scv->m_window != NULL)
	{
		glfwDestroyWindow(scv->m_window);
		scv->m_window = NULL;
	}

	// Terminates GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
}

void ConversionProcessor::clear()
{
	fullBbox.isInitialized = false;
	fullBbox.minX = fullBbox.minY = fullBbox.minZ = 10E9;
	fullBbox.maxX = fullBbox.maxY = fullBbox.maxZ = -10E9;

	attributes.clear();

	if (bResponsibleDisposingGeometries)
	{
		size_t meshCount = allMeshes.size();
		for (size_t i = 0; i < meshCount; i++)
			delete allMeshes[i];
	}

	allMeshes.clear();

	thisSpatialOctree.clear();

	allTextureInfo.clear();

	if (!resizedTextures.empty())
	{
		std::map<std::string, unsigned char*>::iterator itr = resizedTextures.begin();
		for (; itr != resizedTextures.end(); itr++)
			stbi_image_free(itr->second);

		resizedTextures.clear();

		allTextureWidths.clear();
		allTextureHeights.clear();
	}

	settings.clearNsmSettings();

	std::map<unsigned char, gaia3d::TrianglePolyhedron*>::iterator iterNetSurfaceMeshes = netSurfaceMeshes.begin();
	for (; iterNetSurfaceMeshes != netSurfaceMeshes.end(); iterNetSurfaceMeshes++)
		delete iterNetSurfaceMeshes->second;
	netSurfaceMeshes.clear();

	std::map<unsigned char, unsigned char*>::iterator iterNetSurfaceTextures = netSurfaceTextures.begin();
	for (; iterNetSurfaceTextures != netSurfaceTextures.end(); iterNetSurfaceTextures++)
		delete[] iterNetSurfaceTextures->second;
	netSurfaceTextures.clear();

	netSurfaceTextureWidth.clear();
	netSurfaceTextureHeight.clear();

	std::map<unsigned char, gaia3d::TrianglePolyhedron*>::iterator iterSkinMeshes = skinMeshes.begin();
	for (; iterSkinMeshes != skinMeshes.end(); iterSkinMeshes++)
		delete iterSkinMeshes->second;
	skinMeshes.clear();

	std::map<unsigned char, unsigned char*>::iterator iterSkinMeshTextures = skinMeshTextures.begin();
	for (; iterSkinMeshTextures != skinMeshTextures.end(); iterSkinMeshTextures++)
		delete[] iterSkinMeshTextures->second;
	skinMeshTextures.clear();

	skinMeshTextureWidth.clear();
	skinMeshTextureHeight.clear();

	delete[] thumbnail;
	thumbnail = NULL;

	bResponsibleDisposingGeometries = false;
}

void ConversionProcessor::changeSceneControlVariables()
{
	// TODO(khj 20170210) : NYI 여기서 SceneControlVariables를 바꿀 수 있다.
}

void ConversionProcessor::addAttribute(std::string key, std::string value)
{
	attributes.insert(std::map<std::string, std::string>::value_type(key, value));
}

bool ConversionProcessor::proceedConversion(std::vector<gaia3d::TrianglePolyhedron*>& originalMeshes,
	std::map<std::string, std::string>& originalTextureInfo)
{
	switch (settings.meshType)
	{
	case 0:
		convertSemanticData(originalMeshes, originalTextureInfo);
		return true;
	case 1:
		convertSingleRealisticMesh(originalMeshes, originalTextureInfo);
		return true;
	case 2:
		convertSplittedRealisticMesh(originalMeshes, originalTextureInfo);
		return true;
	case 3:
		convertPointCloud(originalMeshes);
		return true;
	default:
	{
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(INVAID_ORIGINAL_MESH_TYPE), false);
		char meshTypeString[256];
		memset(meshTypeString, 0x00, 256);
		sprintf(meshTypeString, "injected mesh type : %d", settings.meshType);
		LogWriter::getLogWriter()->addContents(std::string(meshTypeString), true);
		// replaced
		LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
		LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string() + std::string(INVAID_ORIGINAL_MESH_TYPE) + std::string(meshTypeString));

	}
	return false;
	}

	return true;
}

void ConversionProcessor::convertSplittedRealisticMesh(std::vector<gaia3d::TrianglePolyhedron*>& originalMeshes,
	std::map<std::string, std::string>& originalTextureInfo)
{
	if (settings.nsmSettings.empty())
		settings.fillNsmSettings(settings.netSurfaceMeshSettingIndex);

	// copy data from original to this container
	allMeshes.insert(allMeshes.end(), originalMeshes.begin(), originalMeshes.end());

	// copy texture info
	if (!originalTextureInfo.empty())
		allTextureInfo.insert(originalTextureInfo.begin(), originalTextureInfo.end());

	// calculate original bounding box
	calculateBoundingBox(allMeshes, fullBbox);

	/*
	// change x and y value of all vertex positions such that their origin coincides with the center of bounding box footprint
	if (settings.bAlignPositionToCenter)
	{
		changeXYPlaneCoordinateToRelativeCoordinateToBoundingBoxFootprintCenter(allMeshes, fullBbox);
		printf("[Info]Original coordinate is changed to a coordinate relative to the center of XY-plane projection of bounding box.\n");
	}
	*/

	// calculate plane normals and align them to their vertex normals
	trimVertexNormals(allMeshes);
	printf("[Info]Vertex trimming done.\n");

	// remove duplicated vertices and overlapping triangles
	size_t meshCount = allMeshes.size();
	size_t oldTriangleCount = 0;
	size_t oldVertexCount = 0;
	for (size_t i = 0; i < meshCount; i++)
	{
		oldTriangleCount += allMeshes[i]->getSurfaces()[0]->getTriangles().size();
		oldVertexCount += allMeshes[i]->getVertices().size();
	}
	printf("[Info]%zd vertices and %zd triangles are in original meshes.\n", oldVertexCount, oldTriangleCount);

	dropTrianglesOfSmallSizedEdge(allMeshes, 0.005);
	size_t triangleCount = 0;
	size_t vertexCount = 0;
	for (size_t i = 0; i < meshCount; i++)
	{
		triangleCount += allMeshes[i]->getSurfaces()[0]->getTriangles().size();
		vertexCount += allMeshes[i]->getVertices().size();
	}
	printf("[Info]Small edge triangle removal done. %zd vertices and %zd triangles were removed.\n", oldVertexCount - vertexCount, oldTriangleCount - triangleCount);

	oldTriangleCount = triangleCount;
	oldVertexCount = vertexCount;

	removeDuplicatedVerticesAndOverlappingTriangles(allMeshes, true, false);

	triangleCount = 0;
	vertexCount = 0;
	for (size_t i = 0; i < meshCount; i++)
	{
		triangleCount += allMeshes[i]->getSurfaces()[0]->getTriangles().size();
		vertexCount += allMeshes[i]->getVertices().size();
	}

	printf("[Info]Duplicated vertex and overlapped triangle removal done. %zd vertices and %zd triangles were removed.\n", oldVertexCount - vertexCount, oldTriangleCount - triangleCount);
	printf("[Info]%zd vertices and %zd triangles are survived.\n", vertexCount, triangleCount);


	// determine  which surfaces are exteriors
	if (settings.bExtractExterior)
		determineWhichSurfacesAreExterior(allMeshes, fullBbox);

	// make model-reference relationship
	determineModelAndReference(allMeshes);

	size_t modelCount = 0;
	meshCount = allMeshes.size();
	for (size_t i = 0; i < meshCount; i++)
	{
		if (allMeshes[i]->getReferenceInfo().model == NULL)
			modelCount++;
	}
	printf("[Info]Model/reference detection done. %zd models out of %zd meshes detected.\n", modelCount, allMeshes.size());

	// make VBO
	makeVboObjects(allMeshes);
	printf("[Info]VBO of each mesh created.\n");

	// make generic spatial octree
	assignReferencesIntoEachSpatialOctrees(thisSpatialOctree, allMeshes, fullBbox, false, settings.leafSpatialOctreeSize);
	printf("[Info]Mesh distribution on each octree done.\n");

	// make visibility indices
	if (settings.bOcclusionCulling)
	{
		// exterior extraction is necessary for occlusion culling
		// so, if it is not done, do it before occlusion culling
		if (!settings.bExtractExterior)
			determineWhichSurfacesAreExterior(allMeshes, fullBbox);

		std::vector<gaia3d::TrianglePolyhedron*> interiors, exteriors;
		assignReferencesIntoExteriorAndInterior(allMeshes, interiors, exteriors);

		gaia3d::BoundingBox interiorBbox, exteriorBbox;
		calculateBoundingBox(interiors, interiorBbox);
		calculateBoundingBox(exteriors, exteriorBbox);

		// make occlusion culling information
		gaia3d::VisionOctreeBox interiorOcclusionOctree(NULL), exteriorOcclusionOctree(NULL);
		makeOcclusionInformation(allMeshes, interiorOcclusionOctree, exteriorOcclusionOctree, interiorBbox, exteriorBbox);

		// finally, import these information into data groups and interior spatial octree boxes
		applyOcclusionInformationOnSpatialOctree(thisSpatialOctree, interiorOcclusionOctree, exteriorOcclusionOctree);

		printf("[Info]Visibility Indices created.\n");
	}

	bool bMakeTextureCoordinate = allTextureInfo.empty() ? false : true;
	if (bMakeTextureCoordinate)
	{
		// rebuild original texture
		normalizeTextures(allTextureInfo);
		printf("[Info]Original textures are normalized.\n");
		if (resizedTextures.empty())
		{
			printf("[Error]No Texture Normalized!!!\n\n");
		}
	}

	if (settings.bUseNsm)
	{
		std::map<unsigned char, unsigned char> dummy;
		makeNetSurfaceMeshes(thisSpatialOctree, resizedTextures, allTextureWidths, allTextureHeights, dummy);
		printf("[Info]Net Surface Mesh created.\n");
	}
	else
	{
		reuseOriginalMeshForRougherLods(thisSpatialOctree);
		printf("[Info]Rougher LOD created.\n");
	}


	{
		std::map<std::string, unsigned char*>::iterator iter = resizedTextures.begin();
		unsigned int bpp = 4;
		for (; iter != resizedTextures.end(); iter++)
		{
			unsigned char* resizedImage = iter->second;
			unsigned int widthResized = allTextureWidths[iter->first];
			unsigned int heightResized = allTextureHeights[iter->first];

			unsigned int lineSize = widthResized * bpp;
			unsigned char* lineData = new unsigned char[lineSize];
			memset(lineData, 0x00, sizeof(unsigned char) * lineSize);
			for (unsigned int i = 0; i < heightResized / 2; i++)
			{
				unsigned char* upperLine = resizedImage + (lineSize * i);
				unsigned char* lowerLine = resizedImage + (lineSize * (heightResized - i - 1));

				memcpy(lineData, upperLine, sizeof(unsigned char) * lineSize);
				memcpy(upperLine, lowerLine, sizeof(unsigned char) * lineSize);
				memcpy(lowerLine, lineData, sizeof(unsigned char) * lineSize);
			}

			delete[] lineData;
		}
	}
}

void runPreExperiments(std::vector<gaia3d::TrianglePolyhedron*>& meshes, double& area, std::map<size_t, std::vector<double>>& bboxes)
{
	size_t meshCount = meshes.size();
	printf("[INFO] total object count : %zd\n", meshCount);
	area = 0.0;
	size_t totalTriangleCount = 0;
	for (size_t i = 0; i < meshCount; i++)
	{
		size_t id = meshes[i]->getId();
		bboxes[id] = std::vector<double>();
		(bboxes[id]).push_back(meshes[i]->getBoundingBox().getXLength());
		(bboxes[id]).push_back(meshes[i]->getBoundingBox().getYLength());
		(bboxes[id]).push_back(meshes[i]->getBoundingBox().getZLength());

		for (size_t j = 0; j < meshes[i]->getSurfaces().size(); j++)
		{
			gaia3d::Surface* surface = meshes[i]->getSurfaces()[j];
			totalTriangleCount += surface->getTriangles().size();
			for (size_t k = 0; k < surface->getTriangles().size(); k++)
			{
				gaia3d::Triangle* triangle = surface->getTriangles()[k];
				gaia3d::Vertex* v0 = triangle->getVertices()[0];
				gaia3d::Vertex* v1 = triangle->getVertices()[1];
				gaia3d::Vertex* v2 = triangle->getVertices()[2];

				double edge1X = v1->position.x - v0->position.x;
				double edge1Y = v1->position.y - v0->position.y;
				double edge1Z = v1->position.z - v0->position.z;
				double edge2X = v2->position.x - v0->position.x;
				double edge2Y = v2->position.y - v0->position.y;
				double edge2Z = v2->position.z - v0->position.z;
				double cx, cy, cz;
				gaia3d::GeometryUtility::crossProduct(edge1X, edge1Y, edge1Z, edge2X, edge2Y, edge2Z, cx, cy, cz);
				gaia3d::Point3D cp;
				cp.set(cx, cy, cz);

				area += cp.magnitude() / 2.0;
			}
		}
	}

	printf("[INFO]total triangle count : %zd\n", totalTriangleCount);
	printf("[INFO]total surface area : %f\n", area);
}

void runPostExperiments(std::vector<gaia3d::OctreeBox*>& octrees, double& area, std::map<size_t, std::vector<double>>& bboxes)
{
	size_t octreeCount = octrees.size();
	size_t totalMeshCount = 0, totalTriangleCount = 0;
	area = 0.0;
	std::map<size_t, bool> marked;
	for (size_t i = 0; i < octreeCount; i++)
	{
		gaia3d::OctreeBox* octree = octrees[i];

		for (size_t j = 0; j < octree->meshes.size(); j++)
		{
			gaia3d::TrianglePolyhedron* reference = octree->meshes[j];
			gaia3d::TrianglePolyhedron* model;
			if (reference->getReferenceInfo().model == NULL)
				model = reference;
			else
				model = reference->getReferenceInfo().model;

			size_t id = reference->getId();

			if (marked.find(id) != marked.end())
			{
				continue;
			}

			totalMeshCount++;

			marked[id] = true;

			bboxes[id] = std::vector<double>();
			gaia3d::BoundingBox bbox;
			for (size_t k = 0; k < model->getVertices().size(); k++)
			{
				gaia3d::Vertex* vertex = model->getVertices()[k];
				gaia3d::Point3D point = (reference->getReferenceInfo().mat) * (vertex->position);
				bbox.addPoint(point.x, point.y, point.z);
			}

			(bboxes[id]).push_back(bbox.getXLength());
			(bboxes[id]).push_back(bbox.getYLength());
			(bboxes[id]).push_back(bbox.getZLength());

			/*(bboxes[id]).push_back(reference->getBoundingBox().getXLength());
			(bboxes[id]).push_back(reference->getBoundingBox().getYLength());
			(bboxes[id]).push_back(reference->getBoundingBox().getZLength());*/

			for (size_t k = 0; k < model->getSurfaces().size(); k++)
			{
				gaia3d::Surface* surface = model->getSurfaces()[k];
				totalTriangleCount += surface->getTriangles().size();
				for (size_t m = 0; m < surface->getTriangles().size(); m++)
				{
					gaia3d::Triangle* triangle = surface->getTriangles()[m];
					gaia3d::Vertex* v0 = triangle->getVertices()[0];
					gaia3d::Vertex* v1 = triangle->getVertices()[1];
					gaia3d::Vertex* v2 = triangle->getVertices()[2];

					double edge1X = v1->position.x - v0->position.x;
					double edge1Y = v1->position.y - v0->position.y;
					double edge1Z = v1->position.z - v0->position.z;
					double edge2X = v2->position.x - v0->position.x;
					double edge2Y = v2->position.y - v0->position.y;
					double edge2Z = v2->position.z - v0->position.z;
					double cx, cy, cz;
					gaia3d::GeometryUtility::crossProduct(edge1X, edge1Y, edge1Z, edge2X, edge2Y, edge2Z, cx, cy, cz);
					gaia3d::Point3D cp;
					cp.set(cx, cy, cz);

					area += cp.magnitude() / 2.0;
				}
			}
		}
	}

	printf("[INFO](processed)total object count : %zd\n", totalMeshCount);
	printf("[INFO](processed)total triangle count : %zd\n", totalTriangleCount);
	printf("[INFO](processed)total surface area : %f\n", area);
}

void compareBboxes(std::map<size_t, std::vector<double>>& before, std::map<size_t, std::vector<double>>& after, size_t& errorCount)
{
	if (before.size() != after.size())
	{
		printf("[INFO] before/after counts are different! %zd to %zd\n", before.size(), after.size());
	}

	errorCount = 0;
	std::map<size_t, std::vector<double>>::iterator beforeItr = before.begin();
	std::vector<size_t> errorId;
	for (; beforeItr != before.end(); beforeItr++)
	{
		if (after.find(beforeItr->first) == after.end())
		{
			printf("[INFO] this object does not exist after processing. %zd\n", beforeItr->first);
			errorCount++;
			continue;
		}

		for (size_t i = 0; i < 3; i++)
		{
			double beforeLength = (beforeItr->second)[i];
			double afterLength = (after[beforeItr->first])[i];

			if (beforeLength < 1E-3 && afterLength < 1E-3)
				continue;

			if (beforeLength - 0.001 * beforeLength > afterLength || beforeLength + 0.001 * beforeLength < afterLength)
			{
				errorCount++;
				errorId.push_back(beforeItr->first);
				break;
			}
		}
	}

	if (errorCount != 0)
	{
		for (size_t i = 0; i < errorId.size(); i++)
		{
			printf("[Info]  #%zd : %f, %f, %f to %f, %f, %f\n", errorId[i], before[errorId[i]][0], before[errorId[i]][1], before[errorId[i]][2], after[errorId[i]][0], after[errorId[i]][1], after[errorId[i]][2]);
		}
	}
}

void ConversionProcessor::convertSemanticData(std::vector<gaia3d::TrianglePolyhedron*>& originalMeshes,
	std::map<std::string, std::string>& originalTextureInfo)
{
	if (settings.nsmSettings.empty())
		settings.fillNsmSettings(settings.netSurfaceMeshSettingIndex);

	// copy data from original to this container
	allMeshes.insert(allMeshes.end(), originalMeshes.begin(), originalMeshes.end());

	// copy texture info
	if (!originalTextureInfo.empty())
		allTextureInfo.insert(originalTextureInfo.begin(), originalTextureInfo.end());

	// TODO check if all texture files are available or not

	// calculate original bounding box
	calculateBoundingBox(allMeshes, fullBbox);

	//// experiment test
	//double originalArea;
	//std::map<size_t, std::vector<double>> bboxes;
	//runPreExperiments(allMeshes, originalArea, bboxes);

	// calculate plane normals and align them to their vertex normals
	trimVertexNormals(allMeshes);
	printf("[Info]Vertex trimming done.\n");

	// determine  which surfaces are exterior
	determineWhichSurfacesAreExterior(allMeshes, fullBbox);
	printf("[Info]Exterior surface detection done.\n");

	// determine which triangles are exterior
	determineWhichTrianglesAreExterior(allMeshes, fullBbox);
	size_t triangleCount = 0;
	size_t exteriorTriangleCount = 0;
	for (size_t i = 0; i < allMeshes.size(); i++)
	{
		size_t surfaceCount = allMeshes[i]->getSurfaces().size();
		for (size_t j = 0; j < surfaceCount; j++)
		{
			gaia3d::Surface* surface = allMeshes[i]->getSurfaces()[j];
			triangleCount += surface->getTriangles().size();
			for (size_t k = 0; k < surface->getTriangles().size(); k++)
			{
				if (surface->getTriangles()[k]->isExterior())
					exteriorTriangleCount++;
			}
		}
	}
	printf("[Info]Exterior triangle detection done. %zd of %zd triangles are exterior.\n", exteriorTriangleCount, triangleCount);

	// make model-reference relationship
	determineModelAndReference(allMeshes);

	size_t modelCount = 0;
	size_t meshCount = allMeshes.size();
	for (size_t i = 0; i < meshCount; i++)
	{
		if (allMeshes[i]->getReferenceInfo().model == NULL)
			modelCount++;
	}
	printf("[Info]Model/reference detection done. %zd models out of %zd meshes detected.\n", modelCount, allMeshes.size());

	// make VBO
	makeVboObjects(allMeshes);
	printf("[Info]VBO of each mesh created.\n");

	// make generic spatial octree
	//assignReferencesIntoEachSpatialOctrees(thisSpatialOctree, allMeshes, fullBbox, false, settings.leafSpatialOctreeSize);
	assignReferencesIntoEachSpatialOctrees(thisSpatialOctree, allMeshes, fullBbox, false, fullBbox.getMaxLength());
	printf("[Info]Mesh distribution on each octree done.\n");

	// make visibility indices
	if (settings.bOcclusionCulling)
	{
		// exterior extraction is necessary for occlusion culling
		// so, if it is not done, do it before occlusion culling
		if (!settings.bExtractExterior)
			determineWhichSurfacesAreExterior(allMeshes, fullBbox);

		std::vector<gaia3d::TrianglePolyhedron*> interiors, exteriors;
		assignReferencesIntoExteriorAndInterior(allMeshes, interiors, exteriors);

		gaia3d::BoundingBox interiorBbox, exteriorBbox;
		calculateBoundingBox(interiors, interiorBbox);
		calculateBoundingBox(exteriors, exteriorBbox);

		// make occlusion culling information
		gaia3d::VisionOctreeBox interiorOcclusionOctree(NULL), exteriorOcclusionOctree(NULL);
		makeOcclusionInformation(allMeshes, interiorOcclusionOctree, exteriorOcclusionOctree, interiorBbox, exteriorBbox);

		// finally, import these information into data groups and interior spatial octree boxes
		applyOcclusionInformationOnSpatialOctree(thisSpatialOctree, interiorOcclusionOctree, exteriorOcclusionOctree);

		printf("[Info]Visibility Indices created.\n");
	}

	bool bMakeTextureCoordinate = allTextureInfo.empty() ? false : true;
	if (bMakeTextureCoordinate)
	{
		// rebuild original texture
		normalizeTextures(allTextureInfo);
		printf("[Info]Original textures are normalized.\n");
		if (resizedTextures.empty())
		{
			printf("[Error]No Texture Normalized!!!\n\n");
		}
	}

	// make pretty lod data structure
	makeSkinMeshes(fullBbox, allMeshes, thisSpatialOctree, resizedTextures, allTextureWidths, allTextureHeights);
	printf("[Info]LOD skin Mesh created.\n");

	// make textures upside-down
	{
		std::map<std::string, unsigned char*>::iterator iter = resizedTextures.begin();
		unsigned int bpp = 4;
		for (; iter != resizedTextures.end(); iter++)
		{
			unsigned char* resizedImage = iter->second;
			unsigned int widthResized = allTextureWidths[iter->first];
			unsigned int heightResized = allTextureHeights[iter->first];

			unsigned int lineSize = widthResized * bpp;
			unsigned char* lineData = new unsigned char[lineSize];
			memset(lineData, 0x00, sizeof(unsigned char) * lineSize);
			for (unsigned int i = 0; i < heightResized / 2; i++)
			{
				unsigned char* upperLine = resizedImage + lineSize * i;
				unsigned char* lowerLine = resizedImage + lineSize * (heightResized - i - 1);

				memcpy(lineData, upperLine, sizeof(unsigned char) * lineSize);
				memcpy(upperLine, lowerLine, sizeof(unsigned char) * lineSize);
				memcpy(lowerLine, lineData, sizeof(unsigned char) * lineSize);
			}

			delete[] lineData;
		}
	}

	//// experiment test
	//std::vector<gaia3d::OctreeBox*> leafBoxes;
	//this->getSpatialOctree()->getAllLeafBoxes(leafBoxes, true);
	//double afterArea;
	//std::map<size_t, std::vector<double>> afterBboxes;
	//runPostExperiments(leafBoxes, afterArea, afterBboxes);

	//size_t errorCount;
	//compareBboxes(bboxes, afterBboxes, errorCount);
	//printf("[INFO] %zd out of %zd objects were mis-transformed.\n", errorCount, bboxes.size());

	//system("pause");
}

//void ConversionProcessor::convertSemanticData(std::vector<gaia3d::TrianglePolyhedron*>& originalMeshes,
//	std::map<std::string, std::string>& originalTextureInfo)
//{
//	if (settings.nsmSettings.empty())
//		settings.fillNsmSettings(settings.netSurfaceMeshSettingIndex);
//
//	// copy data from original to this container
//	allMeshes.insert(allMeshes.end(), originalMeshes.begin(), originalMeshes.end());
//
//	// copy texture info
//	if (!originalTextureInfo.empty())
//		allTextureInfo.insert(originalTextureInfo.begin(), originalTextureInfo.end());
//
//	// calculate original bounding box
//	calculateBoundingBox(allMeshes, fullBbox);
//
//	// change x and y value of all vertex positions such that their origin coincides with the center of bounding box footprint 
//	if (settings.bAlignPositionToCenter)
//	{
//		changeXYPlaneCoordinateToRelativeCoordinateToBoundingBoxFootprintCenter(allMeshes, fullBbox);
//		printf("[Info]Original coordinate is changed to a coordinate relative to the center of XY-plane projection of bounding box.\n");
//	}
//
//	// calculate plane normals and align them to their vertex normals
//	trimVertexNormals(allMeshes);
//	printf("[Info]Vertex trimming done.\n");
//
//	// determine  which surfaces are exteriors
//	if (settings.bExtractExterior)
//	{
//		determineWhichSurfacesAreExterior(allMeshes, fullBbox);
//		printf("[Info]Exterior detection done.\n");
//	}
//
//	// make model-reference relationship
//	determineModelAndReference(allMeshes);
//
//	size_t modelCount = 0;
//	size_t meshCount = allMeshes.size();
//	for (size_t i = 0; i < meshCount; i++)
//	{
//		if (allMeshes[i]->getReferenceInfo().model == NULL)
//			modelCount++;
//	}
//	printf("[Info]Model/reference detection done. %zd models out of %zd meshes detected.\n", modelCount, allMeshes.size());
//
//	// make VBO
//	makeVboObjects(allMeshes);
//	printf("[Info]VBO of each mesh created.\n");
//
//	// make generic spatial octree
//	assignReferencesIntoEachSpatialOctrees(thisSpatialOctree, allMeshes, fullBbox, false, settings.leafSpatialOctreeSize);
//	printf("[Info]Mesh distribution on each octree done.\n");
//
//	// make visibility indices
//	if (settings.bOcclusionCulling)
//	{
//		// exterior extraction is necessary for occlusion culling
//		// so, if it is not done, do it before occlusion culling
//		if (!settings.bExtractExterior)
//			determineWhichSurfacesAreExterior(allMeshes, fullBbox);
//
//		std::vector<gaia3d::TrianglePolyhedron*> interiors, exteriors;
//		assignReferencesIntoExteriorAndInterior(allMeshes, interiors, exteriors);
//
//		gaia3d::BoundingBox interiorBbox, exteriorBbox;
//		calculateBoundingBox(interiors, interiorBbox);
//		calculateBoundingBox(exteriors, exteriorBbox);
//
//		// make occlusion culling information
//		gaia3d::VisionOctreeBox interiorOcclusionOctree(NULL), exteriorOcclusionOctree(NULL);
//		makeOcclusionInformation(allMeshes, interiorOcclusionOctree, exteriorOcclusionOctree, interiorBbox, exteriorBbox);
//
//		// finally, import these information into data groups and interior spatial octree boxes
//		applyOcclusionInformationOnSpatialOctree(thisSpatialOctree, interiorOcclusionOctree, exteriorOcclusionOctree);
//
//		printf("[Info]Visibility Indices created.\n");
//	}
//
//	bool bMakeTextureCoordinate = allTextureInfo.empty() ? false : true;
//	if (bMakeTextureCoordinate)
//	{
//		// rebuild original texture
//		normalizeTextures(allTextureInfo);
//		printf("[Info]Original textures are normalized.\n");
//		if (resizedTextures.empty())
//		{
//			printf("[Error]No Texture Normalized!!!\n\n");
//		}
//	}
//
//	if (settings.bUseNsm)
//	{
//		std::map<unsigned char, unsigned char> dummy;
//		makeNetSurfaceMeshes(thisSpatialOctree, resizedTextures, allTextureWidths, allTextureHeights, dummy);
//		printf("[Info]Net Surface Mesh created.\n");
//	}
//	else
//	{
//		reuseOriginalMeshForRougherLods(thisSpatialOctree);
//		printf("[Info]Rougher LOD created.\n");
//	}
//
//	//if (!settings.bFlipTextureCoordinateV)
//	{
//		std::map<std::string, unsigned char*>::iterator iter = resizedTextures.begin();
//		unsigned int bpp = 4;
//		for (; iter != resizedTextures.end(); iter++)
//		{
//			unsigned char* resizedImage = iter->second;
//			unsigned int widthResized = allTextureWidths[iter->first];
//			unsigned int heightResized = allTextureHeights[iter->first];
//
//			unsigned int lineSize = widthResized * bpp;
//			unsigned char* lineData = new unsigned char[lineSize];
//			memset(lineData, 0x00, sizeof(unsigned char) * lineSize);
//			for (unsigned int i = 0; i < heightResized / 2; i++)
//			{
//				unsigned char* upperLine = resizedImage + lineSize * i;
//				unsigned char* lowerLine = resizedImage + lineSize * (heightResized - i - 1);
//
//				memcpy(lineData, upperLine, sizeof(unsigned char) * lineSize);
//				memcpy(upperLine, lowerLine, sizeof(unsigned char) * lineSize);
//				memcpy(lowerLine, lineData, sizeof(unsigned char) * lineSize);
//			}
//
//			delete[] lineData;
//		}
//	}
//	//system("pause");
//}

void ConversionProcessor::convertSingleRealisticMesh(std::vector<gaia3d::TrianglePolyhedron*>& originalMeshes,
	std::map<std::string, std::string>& originalTextureInfo)
{
	if (settings.nsmSettings.empty())
		settings.fillNsmSettings(settings.netSurfaceMeshSettingIndex);

	// copy data from original to this container
	allMeshes.insert(allMeshes.end(), originalMeshes.begin(), originalMeshes.end());

	// copy texture info
	if (!originalTextureInfo.empty())
		allTextureInfo.insert(originalTextureInfo.begin(), originalTextureInfo.end());

	// calculate original bounding box
	calculateBoundingBox(allMeshes, fullBbox);

	// change x and y value of all vertex positions such that their origin coincides with the center of bounding box footprint 
	if (settings.bAlignPositionToCenter)
	{
		changeXYPlaneCoordinateToRelativeCoordinateToBoundingBoxFootprintCenter(allMeshes, fullBbox);
		printf("[Info]Original coordinate is changed to a coordinate relative to the center of XY-plane projection of bounding box.\n");
	}

	// calculate plane normals and align them to their vertex normals
	trimVertexNormals(allMeshes);
	printf("[Info]Vertex trimming done.\n");

	// split original mesh into each leaf spatial octree
	splitOriginalMeshIntoEachSpatialOctrees(thisSpatialOctree, allMeshes, fullBbox, false, settings.leafSpatialOctreeSize, false);

	// collect newly created meshes into the container
	allMeshes.clear();
	std::vector<gaia3d::OctreeBox*> leafOctrees;
	thisSpatialOctree.getAllLeafBoxes(leafOctrees, true);
	size_t leafOctreeCount = leafOctrees.size();
	printf("[Info]Mesh split done. octree count : %zd\n", leafOctreeCount);
	for (size_t i = 0; i < leafOctreeCount; i++)
	{
		size_t meshCount = leafOctrees[i]->meshes.size();
		for (size_t j = 0; j < meshCount; j++)
		{
			leafOctrees[i]->meshes[j]->setId(allMeshes.size());
			calculateBoundingBox(leafOctrees[i]->meshes[j]);
			allMeshes.push_back(leafOctrees[i]->meshes[j]);
		}
	}

	// determine  which surfaces are exteriors
	if (settings.bExtractExterior)
	{
		determineWhichSurfacesAreExterior(allMeshes, fullBbox);
		printf("[Info]Exterior detection done.\n");
	}

	// make model-reference relationship
	determineModelAndReference(allMeshes);
	size_t modelCount = 0;
	size_t meshCount = allMeshes.size();
	for (size_t i = 0; i < meshCount; i++)
	{
		if (allMeshes[i]->getReferenceInfo().model == NULL)
			modelCount++;
	}
	printf("[Info]Model/reference detection done. %zd models out of %zd meshes detected.\n", modelCount, allMeshes.size());

	// drop small-sized edge triangles
	meshCount = allMeshes.size();
	size_t oldTriangleCount = 0;
	size_t oldVertexCount = 0;
	for (size_t i = 0; i < meshCount; i++)
	{
		oldTriangleCount += allMeshes[i]->getSurfaces()[0]->getTriangles().size();
		oldVertexCount += allMeshes[i]->getVertices().size();
	}
	printf("[Info]%zd vertices and %zd triangles are in original meshes.\n", oldVertexCount, oldTriangleCount);

	dropTrianglesOfSmallSizedEdge(allMeshes, 0.005);
	size_t triangleCount = 0;
	size_t vertexCount = 0;
	for (size_t i = 0; i < meshCount; i++)
	{
		triangleCount += allMeshes[i]->getSurfaces()[0]->getTriangles().size();
		vertexCount += allMeshes[i]->getVertices().size();
	}
	printf("[Info]Small edge triangle removal done. %zd vertices and %zd triangles were removed.\n", oldVertexCount - vertexCount, oldTriangleCount - triangleCount);

	// remove duplicated vertices and superimposing triangles
	oldTriangleCount = triangleCount;
	oldVertexCount = vertexCount;

	removeDuplicatedVerticesAndOverlappingTriangles(allMeshes, true, false);
	triangleCount = 0;
	vertexCount = 0;
	for (size_t i = 0; i < meshCount; i++)
	{
		triangleCount += allMeshes[i]->getSurfaces()[0]->getTriangles().size();
		vertexCount += allMeshes[i]->getVertices().size();
	}
	printf("[Info]Duplicated vertex and overlapped triangle removal done. %zd vertices and %zd triangles were removed.\n", oldVertexCount - vertexCount, oldTriangleCount - triangleCount);
	printf("[Info]%zd vertices and %zd triangles are survived.\n", vertexCount, triangleCount);

	// make vbo
	makeVboObjects(allMeshes);
	printf("[Info]VBO of each mesh created.\n");

	// normalize texture
	bool bMakeTextureCoordinate = allTextureInfo.empty() ? false : true;
	if (bMakeTextureCoordinate)
	{
		// rebuild original texture
		normalizeTextures(allTextureInfo);
		printf("[Info]Original textures are normalized.\n");
		if (resizedTextures.empty())
		{
			printf("[Error]No Texture Normalized!!!\n\n");
		}
	}

	// make net surface mesh
	std::map<unsigned char, unsigned char> lodMadeOfOriginalMesh;
	lodMadeOfOriginalMesh[2] = 2;
	lodMadeOfOriginalMesh[3] = 3;
	lodMadeOfOriginalMesh[4] = 4;
	lodMadeOfOriginalMesh[5] = 5;
	makeNetSurfaceMeshes(thisSpatialOctree, resizedTextures, allTextureWidths, allTextureHeights, lodMadeOfOriginalMesh);
	printf("[Info]Net Surface Mesh created.\n");

	// make lod texture using original texture
	if (!resizedTextures.empty())
	{
		std::map<std::string, unsigned char*>::iterator iter = resizedTextures.begin();
		unsigned char* originalTexture = iter->second;
		int originalWidth = allTextureWidths[iter->first];
		int originalHeight = allTextureHeights[iter->first];

		makeLodTextureUsingOriginalTextureDirectly(originalTexture, originalWidth, originalHeight,
			lodMadeOfOriginalMesh,
			netSurfaceTextures,
			netSurfaceTextureWidth, netSurfaceTextureHeight);
		printf("[Info]Textures for Net Surface Mesh created.\n");
	}

	// divide large-sized original texture into small-sized ones along octrees 
	// and re-calculate texture coordinates of all meshes
	if (!resizedTextures.empty())
	{
		std::map<std::string, unsigned char*>::iterator iter = resizedTextures.begin();
		unsigned char* originalTexture = iter->second;
		int originalWidth = allTextureWidths[iter->first];
		int originalHeight = allTextureHeights[iter->first];

		resizedTextures.clear();
		allTextureWidths.clear();
		allTextureHeights.clear();

		divideOriginalTextureIntoSmallerSize(originalTexture, originalWidth, originalHeight,
			thisSpatialOctree,
			resizedTextures,
			allTextureWidths, allTextureHeights,
			allTextureInfo);

		if (resizedTextures.empty())
		{
			printf("[Error]No Texture divided!!!\n\n");
		}
		else
		{
			printf("[Info]Original textures are divided into smaller ones.\n");
		}
	}
}

void ConversionProcessor::convertPointCloud(std::vector<gaia3d::TrianglePolyhedron*>& originalMeshes)
{
	if (originalMeshes.size() != 1)
	{
		printf("[Error]The count of polyhedrons MUST be 1 for point cloud.\n");
		return;
	}
	// copy data from original to this container
	allMeshes.insert(allMeshes.end(), originalMeshes.begin(), originalMeshes.end());

	// calculate original bounding box
	calculateBoundingBox(allMeshes, fullBbox);

	// assign points into each octree cube
	std::random_shuffle(allMeshes[0]->getVertices().begin(), allMeshes[0]->getVertices().end());
	assignObjectsIntoEachCubeInPyramid(thisSpatialOctree, allMeshes, fullBbox, settings.leafSpatialOctreeSize, false, false);

	// collect newly created meshes
	std::vector<gaia3d::OctreeBox*> allCubes;
	thisSpatialOctree.getAllBoxes(allCubes, true);
	allMeshes.clear();
	size_t cubeCount = allCubes.size();
	printf("[Info]Count of created cubes : %zd\n", cubeCount);
	for (size_t i = 0; i < cubeCount; i++)
	{
		size_t meshCount = allCubes[i]->meshes.size();
		for (size_t j = 0; j < meshCount; j++)
		{
			allCubes[i]->meshes[j]->setId(allMeshes.size());
			calculateBoundingBox(allCubes[i]->meshes[j]);
			allMeshes.push_back(allCubes[i]->meshes[j]);
		}
	}
}

void ConversionProcessor::trimVertexNormals(std::vector<gaia3d::TrianglePolyhedron*>& meshes)
{
	size_t meshCount = meshes.size();
	double anglePNormalAndVNormal0, anglePNormalAndVNormal1, anglePNormalAndVNormal2;
	for (size_t i = 0; i < meshCount; i++)
	{
		if (meshes[i]->doesThisHaveNormals())
		{
			std::vector<gaia3d::Surface*>& surfaces = meshes[i]->getSurfaces();
			size_t surfaceCount = surfaces.size();
			for (size_t j = 0; j < surfaceCount; j++)
			{
				std::vector<gaia3d::Triangle*>& triangles = surfaces[j]->getTriangles();
				size_t triangleCount = triangles.size();
				for (size_t k = 0; k < triangleCount; k++)
				{
					gaia3d::Triangle* triangle = triangles[k];
					gaia3d::Point3D planeNormal;
					gaia3d::GeometryUtility::calculatePlaneNormal(triangle->getVertices()[0]->position.x, triangle->getVertices()[0]->position.y, triangle->getVertices()[0]->position.z,
						triangle->getVertices()[1]->position.x, triangle->getVertices()[1]->position.y, triangle->getVertices()[1]->position.z,
						triangle->getVertices()[2]->position.x, triangle->getVertices()[2]->position.y, triangle->getVertices()[2]->position.z,
						planeNormal.x, planeNormal.y, planeNormal.z,
						true);

					if (triangle->getVertices()[0]->normal.magnitude() < 1E-6 ||
						triangle->getVertices()[1]->normal.magnitude() < 1E-6 ||
						triangle->getVertices()[2]->normal.magnitude() < 1E-6)
					{
						*(triangle->getNormal()) = planeNormal;

						triangle->alignVertexNormalsToPlaneNormal();
					}
					else
					{
						anglePNormalAndVNormal0 = 180.0 / M_PI * gaia3d::GeometryUtility::angleBetweenTwoVectors(planeNormal.x, planeNormal.y, planeNormal.z,
							triangle->getVertices()[0]->normal.x,
							triangle->getVertices()[0]->normal.y,
							triangle->getVertices()[0]->normal.z);
						anglePNormalAndVNormal1 = 180.0 / M_PI * gaia3d::GeometryUtility::angleBetweenTwoVectors(planeNormal.x, planeNormal.y, planeNormal.z,
							triangle->getVertices()[1]->normal.x,
							triangle->getVertices()[1]->normal.y,
							triangle->getVertices()[1]->normal.z);
						anglePNormalAndVNormal2 = 180.0 / M_PI * gaia3d::GeometryUtility::angleBetweenTwoVectors(planeNormal.x, planeNormal.y, planeNormal.z,
							triangle->getVertices()[2]->normal.x,
							triangle->getVertices()[2]->normal.y,
							triangle->getVertices()[2]->normal.z);


						if (anglePNormalAndVNormal0 > 90.0 || anglePNormalAndVNormal1 > 90.0 || anglePNormalAndVNormal2 > 90.0)
						{
							size_t tempIndex = triangle->getVertexIndices()[0];
							triangle->getVertexIndices()[0] = triangle->getVertexIndices()[1];
							triangle->getVertexIndices()[1] = tempIndex;

							gaia3d::Vertex* tempVertex = triangle->getVertices()[0];
							triangle->getVertices()[0] = triangle->getVertices()[1];
							triangle->getVertices()[1] = tempVertex;

							triangle->setNormal(-planeNormal.x, -planeNormal.y, -planeNormal.z);
						}
						else
							*(triangle->getNormal()) = planeNormal;
					}
				}
			}
		}
		else
		{
			std::vector<gaia3d::Surface*>& surfaces = meshes[i]->getSurfaces();
			size_t surfaceCount = surfaces.size();
			for (size_t j = 0; j < surfaceCount; j++)
			{
				std::vector<gaia3d::Triangle*>& triangles = surfaces[j]->getTriangles();
				size_t triangleCount = triangles.size();
				for (size_t k = 0; k < triangleCount; k++)
				{
					gaia3d::Triangle* triangle = triangles[k];
					gaia3d::Point3D planeNormal;
					gaia3d::GeometryUtility::calculatePlaneNormal(triangle->getVertices()[0]->position.x, triangle->getVertices()[0]->position.y, triangle->getVertices()[0]->position.z,
						triangle->getVertices()[1]->position.x, triangle->getVertices()[1]->position.y, triangle->getVertices()[1]->position.z,
						triangle->getVertices()[2]->position.x, triangle->getVertices()[2]->position.y, triangle->getVertices()[2]->position.z,
						planeNormal.x, planeNormal.y, planeNormal.z,
						true);

					*(triangle->getNormal()) = planeNormal;

					triangle->alignVertexNormalsToPlaneNormal();
				}
			}

			meshes[i]->setHasNormals(true);
		}
	}
}

void ConversionProcessor::calculateBoundingBox(std::vector<gaia3d::TrianglePolyhedron*>& meshes, gaia3d::BoundingBox& bbox)
{
	size_t meshCount = meshes.size();

	for (size_t i = 0; i < meshCount; i++)
	{
		calculateBoundingBox(meshes[i]);

		bbox.addBox(meshes[i]->getBoundingBox());
	}
}

void ConversionProcessor::determineWhichSurfacesAreExterior(std::vector<gaia3d::TrianglePolyhedron*>& meshes, gaia3d::BoundingBox& bbox)
{

	// allocate a unique color to each surface in all meshes
	std::vector<gaia3d::Surface*> allSurfaces;
	std::vector<gaia3d::ColorU4> allColors;
	size_t meshCount = meshes.size();
	size_t prevSurfaceCount, addedSurfaceCount;
	for (size_t i = 0; i < meshCount; i++)
	{
		prevSurfaceCount = allSurfaces.size();
		addedSurfaceCount = meshes[i]->getSurfaces().size();
		allSurfaces.insert(allSurfaces.end(), meshes[i]->getSurfaces().begin(), meshes[i]->getSurfaces().end());

		for (size_t j = prevSurfaceCount; j < prevSurfaceCount + addedSurfaceCount; j++)
		{
			allColors.push_back(MakeColorU4(GetRedValue(j), GetGreenValue(j), GetBlueValue(j)));
		}
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	scv->tp_projection = PROJECTION_ORTHO;

	defaultSpaceSetupForVisualization(scv->m_width, scv->m_height);

	scv->m_xRot = -60.0; scv->m_yRot = 0.0; scv->m_zRot = 22.5;
	for (int i = 0; i < 8; i++)
	{
		this->setupPerspectiveViewSetting(bbox);
		this->checkIfEachSurfaceIsExterior(allSurfaces, allColors);
		scv->m_zRot += 45.0;
	}

	scv->m_xRot = -90.0; scv->m_yRot = 0.0; scv->m_zRot = 22.5;
	for (int i = 0; i < 8; i++)
	{
		this->setupPerspectiveViewSetting(bbox);
		this->checkIfEachSurfaceIsExterior(allSurfaces, allColors);
		scv->m_zRot += 45.0;
	}

	/*scv->m_xRot = -120.0; scv->m_yRot = 0.0; scv->m_zRot = 22.5;
	for (int i = 0; i < 8; i++)
	{
		this->setupPerspectiveViewSetting(bbox);
		this->checkIfEachSurfaceIsExterior(allSurfaces, allColors);
		scv->m_zRot += 45.0;
	}*/

	scv->tp_projection = PROJECTION_PERSPECTIVE;
	defaultSpaceSetupForVisualization(scv->m_width, scv->m_height);
}

void ConversionProcessor::determineWhichTrianglesAreExterior(std::vector<gaia3d::TrianglePolyhedron*>& meshes, gaia3d::BoundingBox& bbox)
{
	std::vector<gaia3d::Surface*> surfaces;

	size_t meshCount = meshes.size();
	for (size_t i = 0; i < meshCount; i++)
	{
		gaia3d::TrianglePolyhedron* mesh = meshes[i];
		size_t surfaceCount = mesh->getSurfaces().size();
		for (size_t j = 0; j < surfaceCount; j++)
		{
			gaia3d::Surface* surface = mesh->getSurfaces()[j];

			if (surface->isExterior())
				surfaces.push_back(surface);
		}
	}

	if (surfaces.empty())
		return;

	std::vector<gaia3d::Triangle*> triangles;
	std::vector<gaia3d::ColorU4> allColors;
	size_t prevTriangleCount, addedTriangleCount;
	size_t surfaceCount = surfaces.size();
	for (size_t i = 0; i < surfaceCount; i++)
	{
		gaia3d::Surface* surface = surfaces[i];

		prevTriangleCount = triangles.size();
		addedTriangleCount = surface->getTriangles().size();
		triangles.insert(triangles.end(), surface->getTriangles().begin(), surface->getTriangles().end());

		for (size_t j = prevTriangleCount; j < prevTriangleCount + addedTriangleCount; j++)
		{
			allColors.push_back(MakeColorU4(GetRedValue(j), GetGreenValue(j), GetBlueValue(j)));
		}
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	scv->tp_projection = PROJECTION_ORTHO;

	defaultSpaceSetupForVisualization(scv->m_width, scv->m_height);

	scv->m_xRot = -60.0; scv->m_yRot = 0.0; scv->m_zRot = 22.5;
	for (int i = 0; i < 8; i++)
	{
		this->setupPerspectiveViewSetting(bbox);
		this->checkIfEachTriangleIsExterior(triangles, allColors);
		scv->m_zRot += 45.0;
	}

	scv->m_xRot = -90.0; scv->m_yRot = 0.0; scv->m_zRot = 22.5;
	for (int i = 0; i < 8; i++)
	{
		this->setupPerspectiveViewSetting(bbox);
		this->checkIfEachTriangleIsExterior(triangles, allColors);
		scv->m_zRot += 45.0;
	}

	scv->tp_projection = PROJECTION_PERSPECTIVE;
	defaultSpaceSetupForVisualization(scv->m_width, scv->m_height);
}

void ConversionProcessor::makeVboObjects(std::vector<gaia3d::TrianglePolyhedron*>& meshes, bool bBind)
{
	size_t meshCount = meshes.size();
	size_t surfaceCount, triangleCount;
	gaia3d::Vbo* vbo;
	gaia3d::Vertex** vertices;
	std::vector<gaia3d::Triangle*> sortedTriangles;
	unsigned char sizeLevels = TriangleSizeLevels;
	double sizeThresholds[TriangleSizeLevels] = TriangleSizeThresholds;
	unsigned int eachSizeStartingIndices[TriangleSizeLevels];
	for (size_t i = 0; i < meshCount; i++)
	{
		gaia3d::TrianglePolyhedron* mesh = meshes[i];

		vbo = new gaia3d::Vbo;
		mesh->getVbos().push_back(vbo);

		std::map<gaia3d::Vertex*, size_t> addedVertices;

		surfaceCount = mesh->getSurfaces().size();
		std::vector<gaia3d::Triangle*> totalTriangles;
		for (size_t j = 0; j < surfaceCount; j++)
		{
			totalTriangles.insert(totalTriangles.end(), mesh->getSurfaces()[j]->getTriangles().begin(), mesh->getSurfaces()[j]->getTriangles().end());
		}

		//for(size_t j = 0;j < surfaceCount; j++)
		{
			sortedTriangles.clear();
			//sortTrianglesBySize(mesh->getSurfaces()[j]->getTriangles(), sizeLevels, sizeThresholds, sortedTriangles, eachSizeStartingIndices);
			sortTrianglesBySize(totalTriangles, sizeLevels, sizeThresholds, sortedTriangles, eachSizeStartingIndices);

			triangleCount = sortedTriangles.size();
			for (size_t k = 0; k < triangleCount; k++)
			{
				if (vbo->vertices.size() >= VboVertexMaxCount)
				{
					// have to make new vbo when vertex count of current vbo is over limitation of vertex count.
					// before making newer vbo, have to insert index markers into current vbo with sorted index markers along triangle edge size.
					for (size_t m = 0; m < TriangleSizeLevels; m++)
					{
						if ((eachSizeStartingIndices[m] * 3) > vbo->indices.size())
						{
							for (size_t n = m; n < TriangleSizeLevels; n++)
							{
								vbo->indexMarker[n] = (unsigned int)vbo->indices.size();
								eachSizeStartingIndices[n] -= (unsigned int)vbo->indices.size() / 3;
							}
						}
						else
						{
							vbo->indexMarker[m] = eachSizeStartingIndices[m] * 3;
							eachSizeStartingIndices[m] = 0;
						}
					}

					vbo = new gaia3d::Vbo;
					mesh->getVbos().push_back(vbo);

					addedVertices.clear();
				}

				vertices = sortedTriangles[k]->getVertices();
				for (size_t m = 0; m < 3; m++)
				{
					if (addedVertices.find(vertices[m]) == addedVertices.end())
					{
						vbo->indices.push_back((unsigned short)vbo->vertices.size());
						addedVertices.insert(std::map<gaia3d::Vertex*, size_t>::value_type(vertices[m], vbo->vertices.size()));
						vbo->vertices.push_back(vertices[m]);
					}
					else
					{
						vbo->indices.push_back((unsigned short)addedVertices[vertices[m]]);
					}
				}
			}

			// have to insert index markers into the last created vbo
			for (size_t m = 0; m < TriangleSizeLevels; m++)
			{
				if ((eachSizeStartingIndices[m] * 3) > vbo->indices.size())
				{
					for (size_t n = m; n < TriangleSizeLevels; n++)
					{
						vbo->indexMarker[n] = (unsigned int)vbo->indices.size();
						eachSizeStartingIndices[n] -= (unsigned int)vbo->indices.size() / 3;
					}
				}
				else
				{
					vbo->indexMarker[m] = eachSizeStartingIndices[m] * 3;
					eachSizeStartingIndices[m] = 0;
				}
			}
		}

		if (bBind)
		{
			// TODO(khj 20170323) : NYI if we have to use gpu for off-screen rendering later, have to implement it here
		}
	}
}


void ConversionProcessor::determineModelAndReference(std::vector<gaia3d::TrianglePolyhedron*>& meshes)
{
	size_t meshCount = meshes.size();
	gaia3d::TrianglePolyhedron* mesh, * model, * candidate;
	gaia3d::Matrix4 matrix;
	size_t modelId = 0;
	for (size_t i = 0; i < meshCount; i++)
	{
		mesh = meshes[i];
		// check if this mesh can be a model
		if (mesh->getReferenceInfo().model != NULL)
			continue;

		// at this time, this mesh is a model
		model = mesh;
		model->getReferenceInfo().modelIndex = modelId;

		// using this model, find and mark all references
		for (size_t j = i + 1; j < meshCount; j++)
		{
			mesh = meshes[j];
			// check if this mesh is refering other model
			if (mesh->getReferenceInfo().model != NULL)
				continue;

			// at this time, this mesh is a candidate which can refer this model
			candidate = mesh;

			if (gaia3d::GeometryUtility::areTwoCongruentWithEachOther(candidate, model, &matrix, 0.0, gaia3d::GeometryUtility::POLYHEDRON))
			{
				candidate->getReferenceInfo().model = model;
				candidate->getReferenceInfo().mat.set(&matrix);
				candidate->getReferenceInfo().modelIndex = modelId;
			}
		}

		modelId++;
	}
}

void ConversionProcessor::assignReferencesIntoExteriorAndInterior(std::vector<gaia3d::TrianglePolyhedron*>& meshes,
	std::vector<gaia3d::TrianglePolyhedron*>& interiors,
	std::vector<gaia3d::TrianglePolyhedron*>& exteriors)
{
	size_t meshCount = meshes.size();
	gaia3d::TrianglePolyhedron* mesh;
	for (size_t i = 0; i < meshCount; i++)
	{
		mesh = meshes[i];

		if (mesh->doesHaveAnyExteriorSurface())
			exteriors.push_back(mesh);
		else
			interiors.push_back(mesh);
	}
}

void ConversionProcessor::assignReferencesIntoEachSpatialOctrees(gaia3d::SpatialOctreeBox& spatialOctree,
	std::vector<gaia3d::TrianglePolyhedron*>& meshes,
	gaia3d::BoundingBox& bbox,
	bool bFixedDepth,
	double leafBoxSize,
	bool bRefOnOnlyOneLeaf)
{
	// prepare spatial indexing
	if (bbox.isInitialized)
	{
		double maxLength = bbox.getMaxLength();
		spatialOctree.setSize(bbox.minX, bbox.minY, bbox.minZ, bbox.minX + maxLength, bbox.minY + maxLength, bbox.minZ + maxLength);
		spatialOctree.meshes.insert(spatialOctree.meshes.end(), meshes.begin(), meshes.end());

		if (bFixedDepth)
		{
			spatialOctree.makeTree(SpatialIndexingDepth);

			spatialOctree.distributeMeshesIntoEachChildren(bRefOnOnlyOneLeaf);
		}
		else
		{
			spatialOctree.makeTreeOfUnfixedDepth(leafBoxSize, bRefOnOnlyOneLeaf);
		}

		spatialOctree.setOctreeId();
	}
}

void ConversionProcessor::splitOriginalMeshIntoEachSpatialOctrees(gaia3d::SpatialOctreeBox& spatialOctree,
	std::vector<gaia3d::TrianglePolyhedron*>& meshes,
	gaia3d::BoundingBox& bbox,
	bool bFixedDepth,
	double leafBoxSize,
	bool bAllowDuplication)
{
	if (!bbox.isInitialized)
		return;

	// not yet support fixed depth
	if (bFixedDepth)
		return;

	double maxLength = bbox.getMaxLength();
	spatialOctree.setSize(bbox.minX, bbox.minY, bbox.minZ, bbox.minX + maxLength, bbox.minY + maxLength, bbox.minZ + maxLength);
	spatialOctree.meshes.insert(spatialOctree.meshes.end(), meshes.begin(), meshes.end());

	spatialOctree.makeTreeOfUnfixedDepth(leafBoxSize, !bAllowDuplication, true);

	spatialOctree.setOctreeId();
}

void ConversionProcessor::assignObjectsIntoEachCubeInPyramid(gaia3d::SpatialOctreeBox& spatialOctree,
	std::vector<gaia3d::TrianglePolyhedron*>& meshes,
	gaia3d::BoundingBox& bbox,
	double leafBoxSize,
	bool bAllowDuplication,
	bool bBasedOnMesh)
{
	if (!bbox.isInitialized)
		return;

	double maxLength = bbox.getMaxLength();
	spatialOctree.setSize(bbox.minX, bbox.minY, bbox.minZ, bbox.minX + maxLength, bbox.minY + maxLength, bbox.minZ + maxLength);
	spatialOctree.meshes.insert(spatialOctree.meshes.end(), meshes.begin(), meshes.end());

	spatialOctree.makeFullCubePyramid(leafBoxSize, !bAllowDuplication, bBasedOnMesh);

	spatialOctree.setOctreeId();
}

void ConversionProcessor::makeOcclusionInformation(std::vector<gaia3d::TrianglePolyhedron*>& meshes,
	gaia3d::VisionOctreeBox& interiorOcclusionOctree,
	gaia3d::VisionOctreeBox& exteriorOcclusionOctree,
	gaia3d::BoundingBox& interiorBbox,
	gaia3d::BoundingBox& exteriorBbox)
{
	// allocate a unique color to each mesh
	std::vector<gaia3d::ColorU4> allColors;
	size_t meshCount = meshes.size();
	for (size_t i = 0; i < meshCount; i++)
		allColors.push_back(MakeColorU4(GetRedValue(i), GetGreenValue(i), GetBlueValue(i)));

	// 2 occlusion culling processes should be performed.
	// one for interior objects and the other for exterior objects.

	// before occlusion culling processes, make display list of all meshes with index colors
	glDisable(GL_TEXTURE_2D);
	makeDisplayListOfMeshes(meshes, allColors);

	// occlusion culling for interior
	if (interiorBbox.isInitialized)
	{
		interiorOcclusionOctree.setSize(interiorBbox.minX, interiorBbox.minY, interiorBbox.minZ, interiorBbox.maxX, interiorBbox.maxY, interiorBbox.maxZ);
		interiorOcclusionOctree.makeTree(settings.interiorVisibilityIndexingOctreeDepth);
		makeVisibilityIndices(interiorOcclusionOctree,
			meshes,
			settings.interiorVisibilityIndexingCameraStep,
			settings.interiorVisibilityIndexingCameraStep,
			settings.interiorVisibilityIndexingCameraStep,
			NULL);
	}

	// occlusion culling for exterior
	if (exteriorBbox.isInitialized)
	{
		double bufferLength = exteriorBbox.getMaxLength() * 0.5;
		exteriorOcclusionOctree.setSize(exteriorBbox.minX - bufferLength, exteriorBbox.minY - bufferLength, exteriorBbox.minZ - bufferLength,
			exteriorBbox.maxX + bufferLength, exteriorBbox.maxY + bufferLength, exteriorBbox.maxZ + bufferLength);
		exteriorOcclusionOctree.makeTree(settings.exteriorVisibilityIndexingOctreeDepth);
		makeVisibilityIndices(exteriorOcclusionOctree,
			meshes,
			settings.exteriorVisibilityIndexingCameraStep,
			settings.exteriorVisibilityIndexingCameraStep,
			settings.exteriorVisibilityIndexingCameraStep,
			&interiorOcclusionOctree);
	}

	glEnable(GL_TEXTURE_2D);
}

void ConversionProcessor::applyOcclusionInformationOnSpatialOctree(gaia3d::SpatialOctreeBox& spatialOctree,
	gaia3d::VisionOctreeBox& interiorOcclusionOctree,
	gaia3d::VisionOctreeBox& exteriorOcclusionOctree)
{
	gaia3d::VisionOctreeBox* interiorOcclusionInfoOfEachGroup, * exteriorOcclusionInfoOfEachGroup;

	std::vector<gaia3d::OctreeBox*> leafBoxes;
	spatialOctree.getAllLeafBoxes(leafBoxes);
	size_t leafBoxCount = leafBoxes.size();
	for (size_t i = 0; i < leafBoxCount; i++)
	{
		interiorOcclusionInfoOfEachGroup = ((gaia3d::SpatialOctreeBox*)leafBoxes[i])->interiorOcclusionInfo;
		interiorOcclusionInfoOfEachGroup->copyDimensionsFromOtherOctreeBox(interiorOcclusionOctree);
		extractMatchedReferencesFromOcclusionInfo(interiorOcclusionInfoOfEachGroup, interiorOcclusionOctree, leafBoxes[i]->meshes);

		exteriorOcclusionInfoOfEachGroup = ((gaia3d::SpatialOctreeBox*)leafBoxes[i])->exteriorOcclusionInfo;
		exteriorOcclusionInfoOfEachGroup->copyDimensionsFromOtherOctreeBox(exteriorOcclusionOctree);
		extractMatchedReferencesFromOcclusionInfo(exteriorOcclusionInfoOfEachGroup, exteriorOcclusionOctree, leafBoxes[i]->meshes);
	}
}

/**
 * resize texture
 */
void ConversionProcessor::normalizeTextures(std::map<std::string, std::string>& textureInfo)
{
	std::map<std::string, std::string>::iterator itr = textureInfo.begin();
	std::string fileExt;
	std::string fileName, fullPath;
	for (; itr != textureInfo.end(); itr++)
	{
		fileName = itr->first;
		fullPath = itr->second;

		std::string::size_type dotPosition = fileName.rfind(".");
		if (dotPosition == std::string::npos)
			continue;

		std::string fileExt = fileName.substr(dotPosition + 1, fileName.length() - dotPosition - 1);
		std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), towlower);

		//std::string singleFullPath(gaia3d::ws2s(fullPath.c_str()));
		std::string singleFullPath(fullPath.c_str());

		if (fileExt.compare("jpg") != 0 && fileExt.compare("jpeg") != 0 && fileExt.compare("jpe") != 0 &&
			fileExt.compare("png") != 0 && fileExt.compare("gif") != 0 && fileExt.compare("bmp") != 0 &&
			fileExt.compare("tif") != 0 && fileExt.compare("tiff") != 0)
		{
			printf("[WARNING] unsupported image format : %s\n", singleFullPath.c_str());
			continue;
		}
			

		unsigned char* pData = NULL;
		unsigned char* pDest = NULL;
		int bmpWidth = -1, bmpHeight = -1, nrChannels;
		unsigned int bmpWidthResized, bmpHeightResized;

		// loading
		stbi_set_flip_vertically_on_load(true);
		pData = stbi_load(singleFullPath.c_str(), &bmpWidth, &bmpHeight, &nrChannels, 4);
		nrChannels = 4;

		if (bmpWidth <= 0 || bmpHeight <= 0)
		{
			printf("[ERROR] zero width or height : %s\n", singleFullPath.c_str());
			continue;
		}
			
		if (pData == NULL)
		{
			printf("[ERROR] no memory allocated for image : %s\n", singleFullPath.c_str());
			continue;
		}
			

		bmpWidthResized = bmpWidth;
		bmpHeightResized = bmpHeight;

		// width가 2^n 형태가 아니면 2^n 형태로 resize 해야 한다.
		if ((bmpWidth & (bmpWidth - 1)) != 0)
		{
			unsigned int prevPower = 1;
			while (bmpWidth >> prevPower != 1)
				prevPower++;

			unsigned int nextPower = prevPower + 1;
			bmpWidthResized = ((bmpWidth - (1 << prevPower)) > ((1 << nextPower) - bmpWidth)) ? 1 << nextPower : 1 << prevPower;
		}

		// height가 2^n 형태가 아니면 2^n 형태로 resize 해야 한다.
		if ((bmpHeight & (bmpHeight - 1)) != 0)
		{
			unsigned int prevPower = 1;
			while (bmpHeight >> prevPower != 1)
				prevPower++;

			unsigned int nextPower = prevPower + 1;
			bmpHeightResized = ((bmpHeight - (1 << prevPower)) > ((1 << nextPower) - bmpHeight)) ? 1 << nextPower : 1 << prevPower;
		}

		pDest = new unsigned char[bmpWidthResized * bmpHeightResized * nrChannels];
		stbir_resize_uint8(pData, bmpWidth, bmpHeight, 0, pDest, bmpWidthResized, bmpHeightResized, 0, nrChannels);
		stbi_image_free(pData);

		/*
		if (settings.bFlipTextureCoordinateV)
		{
			int lineSize = bmpWidthResized * nrChannels;
			unsigned char* lineData = new unsigned char[lineSize];
			memset(lineData, 0x00, sizeof(unsigned char)*lineSize);
			for (int i = 0; i < bmpHeightResized / 2; i++)
			{
				unsigned char* upperLine = pDest + lineSize * i;
				unsigned char* lowerLine = pDest + lineSize * (bmpHeightResized - i - 1);

				memcpy(lineData, upperLine, sizeof(unsigned char)*lineSize);
				memcpy(upperLine, lowerLine, sizeof(unsigned char)*lineSize);
				memcpy(lowerLine, lineData, sizeof(unsigned char)*lineSize);
			}

			delete[] lineData;
		}
		*/

		// 최종 결과물을 container에 넣는다.
		resizedTextures[itr->first] = pDest;
		allTextureWidths[itr->first] = bmpWidthResized;
		allTextureHeights[itr->first] = bmpHeightResized;
	}
}

void ConversionProcessor::calculateBoundingBox(gaia3d::TrianglePolyhedron* mesh)
{
	if (mesh->getBoundingBox().isInitialized)
		return;

	size_t surfaceCount = mesh->getSurfaces().size();
	for (size_t i = 0; i < surfaceCount; i++)
	{
		gaia3d::Surface* surface = mesh->getSurfaces()[i];
		size_t triangleCount = surface->getTriangles().size();
		for (size_t j = 0; j < triangleCount; j++)
		{
			gaia3d::Triangle* triangle = surface->getTriangles()[j];
			gaia3d::Vertex** vertices = triangle->getVertices();
			mesh->getBoundingBox().addPoint(vertices[0]->position.x, vertices[0]->position.y, vertices[0]->position.z);
			mesh->getBoundingBox().addPoint(vertices[1]->position.x, vertices[1]->position.y, vertices[1]->position.z);
			mesh->getBoundingBox().addPoint(vertices[2]->position.x, vertices[2]->position.y, vertices[2]->position.z);
		}
	}

	if (!mesh->getBoundingBox().isInitialized)
	{
		std::vector<gaia3d::Vertex*>& vertices = mesh->getVertices();
		size_t vertexCount = vertices.size();
		for (size_t j = 0; j < vertexCount; j++)
			mesh->getBoundingBox().addPoint(vertices[j]->position.x, vertices[j]->position.y, vertices[j]->position.z);
	}
}

void ConversionProcessor::defaultSpaceSetupForVisualization(int width, int height)
{
	float ratio = (float)width / height;
	float range = scv->m_nRange;

	scv->m_width = width;
	scv->m_height = height;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Set the viewport 
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();


	if (scv->tp_projection == PROJECTION_PERSPECTIVE)
	{
		//gluPerspective(scv->m_perspective_angle, (GLfloat) scv->m_width /(GLfloat) scv->m_height ,scv->m_perspective_near, scv->m_perspective_far);
		glm::mat4 projection = glm::perspective(glm::radians(scv->m_perspective_angle),
			(double)ratio,
			scv->m_perspective_near,
			scv->m_perspective_far);
		glLoadMatrixf((const GLfloat*)&projection[0][0]);


	}
	else if (scv->tp_projection == PROJECTION_ORTHO)
	{
		if (scv->m_width <= scv->m_height)
			glOrtho(-range, range, -range / ratio, range / ratio, -range * 5000.0f, range * 5000.0f);
		else
			glOrtho(-range * ratio, range * ratio, -range, range, -range * 5000.0f, range * 5000.0f);
	}

	glMatrixMode(GL_MODELVIEW);
}

void ConversionProcessor::setupPerspectiveViewSetting(gaia3d::BoundingBox& bbox)
{
	gaia3d::Point3D centerPoint;
	bbox.getCenterPoint(centerPoint.x, centerPoint.y, centerPoint.z);

	// Ara calculem la posicio de la camara tenint en compte xRot, yRot i zRot.***
	gaia3d::Point3D rotation_vector, new_cam_pos;
	double max_length_bc = bbox.getMaxLength();

	if (scv->tp_projection == PROJECTION_PERSPECTIVE)
	{
		double dist_aux = (max_length_bc / 2.0) / tan(scv->m_perspective_angle / 2.0 * scv->MPI_Div_180);// provisional.***

		gaia3d::Matrix4 mat_xrot, mat_yrot, mat_zrot;
		mat_xrot.rotation(scv->m_xRot * scv->MPI_Div_180, 1.0, 0.0, 0.0);
		mat_yrot.rotation(scv->m_yRot * scv->MPI_Div_180, 0.0, 1.0, 0.0);
		mat_zrot.rotation(scv->m_zRot * scv->MPI_Div_180, 0.0, 0.0, 1.0);
		scv->mat_rot = (mat_xrot * mat_yrot) * mat_zrot;

		scv->m_viewing_direction = scv->mat_rot * scv->m_viewing_direction;

		rotation_vector.set(0.0, 0.0, -1.0);
		rotation_vector = scv->mat_rot * rotation_vector;

		new_cam_pos.set(centerPoint.x - rotation_vector.x * dist_aux,
			centerPoint.y - rotation_vector.y * dist_aux,
			centerPoint.z - rotation_vector.z * dist_aux);

		scv->m_xPos = -new_cam_pos.x;
		scv->m_yPos = -new_cam_pos.y;
		scv->m_zPos = -new_cam_pos.z;
	}
	else if (scv->tp_projection == PROJECTION_ORTHO)
	{
		scv->m_nRange = (float)(max_length_bc * 1.2f) / 2.0f;
		scv->m_xPos = -centerPoint.x;
		scv->m_yPos = -centerPoint.y;
		scv->m_zPos = -centerPoint.z;
		defaultSpaceSetupForVisualization(scv->m_width, scv->m_height);
	}
}

void ConversionProcessor::checkIfEachSurfaceIsExterior(std::vector<gaia3d::Surface*>& surfaces, std::vector<gaia3d::ColorU4>& colors)
{

	size_t last_idx_triSurface = 4294967295;// Max value for an unsigned int.***

	int data_size = scv->m_width * scv->m_height;
	GLuint* data = NULL;

	//GLuint *data = new GLuint[data_size];
	//memset(data, 0x00, data_size * sizeof(GLuint));

	glDisable(GL_LIGHTING);

	this->drawSurfacesWithIndexColor(surfaces, colors);

#if USE_NATIVE_OSMESA
	glfwGetOSMesaColorBuffer(scv->m_window, &(scv->m_width), &(scv->m_height), NULL, (void**)&data);
#else
	data = (GLuint*)malloc(sizeof(GLuint) * data_size);
	if (data)
	{
		glReadPixels(0, 0, scv->m_width, scv->m_height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
#endif

	int pixels_count = data_size;
	size_t triSurfaces_count = surfaces.size();

	// Make a colors counter. if an objects is visible by a little count of pixels, then, this object is not visible.***
	std::map<unsigned int, int> map_triSurf_pixelsCount;
	std::map<unsigned int, int>::iterator it;
	std::vector<unsigned int> idx;

	for (int i = 0; i < pixels_count; i++)
	{
		unsigned int idx_triSurf = (data[i] & 0x00ffffffU);// Original by Hak.***

		if (idx_triSurf < triSurfaces_count)
		{
			// now, search the triSurf inside of the map.***
			it = map_triSurf_pixelsCount.find(idx_triSurf);
			if (it != map_triSurf_pixelsCount.end())
			{
				// found.***
				int current_value = it->second;
				map_triSurf_pixelsCount[idx_triSurf] = current_value + 1;
			}
			else
			{
				// no found.***
				map_triSurf_pixelsCount[idx_triSurf] = 1;
			}
		}
	}

	// Now, for each triSurf in the map, set as visible if his value is bigger than minValue.***
	for (std::map<unsigned int, int>::iterator itera = map_triSurf_pixelsCount.begin(); itera != map_triSurf_pixelsCount.end(); itera++)
	{
		gaia3d::Surface* triSurf = surfaces[itera->first];
		int pixels_counted = itera->second;

		if (pixels_counted > ExteriorDetectionThreshold)
		{
			triSurf->setIsExterior(true);
			colors[itera->first] = MakeColorU4(255, 255, 255);
		}
	}

#ifndef USE_NATIVE_OSMESA
	//delete []data;
	if (data != NULL)	free(data);
#endif

	glfwSwapBuffers(scv->m_window);
	glfwPollEvents();

	glEnable(GL_LIGHTING);
}

void ConversionProcessor::checkIfEachTriangleIsExterior(std::vector<gaia3d::Triangle*>& triangles, std::vector<gaia3d::ColorU4>& colors)
{
	size_t last_idx_triangle = 4294967295;// Max value for an unsigned int.***

	int data_size = scv->m_width * scv->m_height;
	GLuint* data = NULL;

	glDisable(GL_LIGHTING);

	this->drawTrianglesWithIndexColor(triangles, colors);

#if USE_NATIVE_OSMESA
	glfwGetOSMesaColorBuffer(scv->m_window, &(scv->m_width), &(scv->m_height), NULL, (void**)&data);
#else
	data = (GLuint*)malloc(sizeof(GLuint) * data_size);
	if (data)
	{
		glReadPixels(0, 0, scv->m_width, scv->m_height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
#endif

	int pixels_count = data_size;
	size_t triangleCount = triangles.size();

	// Make a colors counter. if a triangle is visible by a little count of pixels, then, this object is not visible.***
	std::map<unsigned int, int> map_triangle_pixelsCount;
	std::map<unsigned int, int>::iterator it;

	for (int i = 0; i < pixels_count; i++)
	{
		unsigned int idx_triangle = (data[i] & 0x00ffffffU);// Original by Hak.***

		if (idx_triangle < triangleCount)
		{
			// now, search the triangle in the map.***
			it = map_triangle_pixelsCount.find(idx_triangle);
			if (it != map_triangle_pixelsCount.end())
			{
				// found.***
				int current_value = it->second;
				map_triangle_pixelsCount[idx_triangle] = current_value + 1;
			}
			else
			{
				// no found.***
				map_triangle_pixelsCount[idx_triangle] = 1;
			}
		}
	}

	// Now, for each triangle in the map, set as visible if his value is bigger than minValue.***
	for (std::map<unsigned int, int>::iterator itera = map_triangle_pixelsCount.begin(); itera != map_triangle_pixelsCount.end(); itera++)
	{
		gaia3d::Triangle* triangle = triangles[itera->first];
		int pixels_counted = itera->second;

		if (pixels_counted > ExteriorDetectionThreshold)
		{
			triangle->setIfExterior(true);
			colors[itera->first] = MakeColorU4(255, 255, 255);
		}
	}

#ifndef USE_NATIVE_OSMESA
	//delete []data;
	if (data != NULL)	free(data);
#endif

	glfwSwapBuffers(scv->m_window);
	glfwPollEvents();

	glEnable(GL_LIGHTING);
}

void ConversionProcessor::drawSurfacesWithIndexColor(std::vector<gaia3d::Surface*>& surfaces, std::vector<gaia3d::ColorU4>& colors)
{
	//glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	// Clear the screen and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Reset the model matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();

	glRotatef((float)scv->m_xRot_aditional, 1.0f, 0.0f, 0.0f);
	glRotatef((float)scv->m_yRot_aditional, 0.0f, 1.0f, 0.0f);
	glRotatef((float)scv->m_zRot_aditional, 0.0f, 0.0f, 1.0f);

	glRotatef((float)scv->m_xRot, 1.0f, 0.0f, 0.0f);
	glRotatef((float)scv->m_yRot, 0.0f, 1.0f, 0.0f);
	glRotatef((float)scv->m_zRot, 0.0f, 0.0f, 1.0f);

	glTranslatef((float)scv->m_xPos, (float)scv->m_yPos, (float)scv->m_zPos);

	size_t triSurfaces_count = surfaces.size();
	GLubyte ubyte_r, ubyte_g, ubyte_b;

	ubyte_r = 0;
	ubyte_g = 0;
	ubyte_b = 0;

	glBegin(GL_TRIANGLES);
	for (size_t i = 0; i < triSurfaces_count; i++)
	{
		gaia3d::Surface* triSurf = surfaces[i];

		ubyte_r = GetRedValue(colors[i]);
		ubyte_g = GetGreenValue(colors[i]);;
		ubyte_b = GetBlueValue(colors[i]);;

		glColor3ub(ubyte_r, ubyte_g, ubyte_b);

		size_t triangles_count = triSurf->getTriangles().size();
		for (size_t j = 0; j < triangles_count; j++)
		{
			gaia3d::Triangle* tri = triSurf->getTriangles()[j];

			glVertex3f((float)tri->getVertices()[0]->position.x, (float)tri->getVertices()[0]->position.y, (float)tri->getVertices()[0]->position.z);
			glVertex3f((float)tri->getVertices()[1]->position.x, (float)tri->getVertices()[1]->position.y, (float)tri->getVertices()[1]->position.z);
			glVertex3f((float)tri->getVertices()[2]->position.x, (float)tri->getVertices()[2]->position.y, (float)tri->getVertices()[2]->position.z);
		}
	}
	glEnd();
	//-----------------------------------------------------------------------------------

	glPopMatrix();
	glFlush();

	//glfwSwapBuffers(scv->m_window);
	//glfwPollEvents();
}

void ConversionProcessor::drawTrianglesWithIndexColor(std::vector<gaia3d::Triangle*>& triangles, std::vector<gaia3d::ColorU4>& colors)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Reset the model matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();

	glRotatef((float)scv->m_xRot_aditional, 1.0f, 0.0f, 0.0f);
	glRotatef((float)scv->m_yRot_aditional, 0.0f, 1.0f, 0.0f);
	glRotatef((float)scv->m_zRot_aditional, 0.0f, 0.0f, 1.0f);

	glRotatef((float)scv->m_xRot, 1.0f, 0.0f, 0.0f);
	glRotatef((float)scv->m_yRot, 0.0f, 1.0f, 0.0f);
	glRotatef((float)scv->m_zRot, 0.0f, 0.0f, 1.0f);

	glTranslatef((float)scv->m_xPos, (float)scv->m_yPos, (float)scv->m_zPos);

	size_t triangleCount = triangles.size();
	GLubyte ubyte_r, ubyte_g, ubyte_b;

	ubyte_r = 0;
	ubyte_g = 0;
	ubyte_b = 0;

	glBegin(GL_TRIANGLES);
	for (size_t i = 0; i < triangleCount; i++)
	{
		gaia3d::Triangle* triangle = triangles[i];

		ubyte_r = GetRedValue(colors[i]);
		ubyte_g = GetGreenValue(colors[i]);
		ubyte_b = GetBlueValue(colors[i]);

		glColor3ub(ubyte_r, ubyte_g, ubyte_b);

		glVertex3f((float)triangle->getVertices()[0]->position.x, (float)triangle->getVertices()[0]->position.y, (float)triangle->getVertices()[0]->position.z);
		glVertex3f((float)triangle->getVertices()[1]->position.x, (float)triangle->getVertices()[1]->position.y, (float)triangle->getVertices()[1]->position.z);
		glVertex3f((float)triangle->getVertices()[2]->position.x, (float)triangle->getVertices()[2]->position.y, (float)triangle->getVertices()[2]->position.z);
	}
	glEnd();
	//-----------------------------------------------------------------------------------

	glPopMatrix();
	glFlush();
}

void ConversionProcessor::makeVisibilityIndices(gaia3d::VisionOctreeBox& octree, std::vector<gaia3d::TrianglePolyhedron*>& meshes,
	float scanStepX, float scanStepY, float scanStepZ,
	gaia3d::VisionOctreeBox* excludedBox)
{
	// Save the current state to return at the final of the process.***
	double currentXPos = scv->m_xPos;
	double currentYPos = scv->m_yPos;
	double currentZPos = scv->m_zPos;

	double currentXRot = scv->m_xRot;
	double currentYRot = scv->m_yRot;
	double currentZRot = scv->m_zRot;


	// temporary setup for space of visualization for offscreen rendering for occlusion culling
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	int screenSize = OcclusionCullingScreenSize; // edge length of screen for offscreen rendering for occlusion culling. we set the screen squared
	//int pixels_count = screenSize * screenSize;
	scv->m_perspective_angle = 90.0;

	glViewport(0, 0, (GLsizei)screenSize, (GLsizei)screenSize); // Set the viewport 

	// Setup perspective perspective matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// gluPerspective(scv->m_perspective_angle, (GLfloat)1.0f , scv->m_perspective_near, scv->m_perspective_far);
	glm::mat4 projection = glm::perspective(glm::radians(scv->m_perspective_angle),
		(double)1.0f,
		scv->m_perspective_near,
		scv->m_perspective_far);
	glLoadMatrixf((const GLfloat*)&projection[0][0]);

	glMatrixMode(GL_MODELVIEW);

	glReadBuffer(GL_BACK);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	// temporary setup - end

	std::vector<gaia3d::OctreeBox*> leafBoxes;
	octree.getAllLeafBoxes(leafBoxes);

	size_t meshCount = meshes.size();
	size_t leafBoxCount = leafBoxes.size();
	gaia3d::VisionOctreeBox* leafBox;
	std::vector<gaia3d::Point3D> scanningPoints;
	size_t pointCount;
	gaia3d::Point3D targetPoint;
	std::map<size_t, size_t> indices;
	for (size_t i = 0; i < leafBoxCount; i++)
	{
		leafBox = (gaia3d::VisionOctreeBox*)leafBoxes[i];
		leafBox->getInternalDivisionPoints(scanningPoints, scanStepX, scanStepY, scanStepZ);
		pointCount = scanningPoints.size();
		for (size_t j = 0; j < pointCount; j++)
		{
			targetPoint = scanningPoints[j];

			if (excludedBox != NULL)
			{
				if (gaia3d::GeometryUtility::isInsideBox(targetPoint.x, targetPoint.y, targetPoint.z,
					excludedBox->minX, excludedBox->minY, excludedBox->minZ,
					excludedBox->maxX, excludedBox->maxY, excludedBox->maxZ))
					continue;
			}

			scv->m_xPos = -targetPoint.x;
			scv->m_yPos = -targetPoint.y;
			scv->m_zPos = -targetPoint.z;

			// FRONT.***
			scv->m_xRot = 0.0; scv->m_yRot = 0.0; scv->m_zRot = 0.0;
			drawAndDetectVisibleColorIndices(indices);

			// TOP.***
			scv->m_xRot = -90.0; scv->m_yRot = 0.0; scv->m_zRot = 0.0;
			drawAndDetectVisibleColorIndices(indices);

			// LEFT.***
			scv->m_xRot = 0.0; scv->m_yRot = -90.0; scv->m_zRot = 0.0;
			drawAndDetectVisibleColorIndices(indices);

			// RIGHT.***
			scv->m_xRot = 0.0; scv->m_yRot = 90.0; scv->m_zRot = 0.0;
			drawAndDetectVisibleColorIndices(indices);

			// REAR.***
			scv->m_xRot = 180.0; scv->m_yRot = 0.0; scv->m_zRot = 0.0;
			drawAndDetectVisibleColorIndices(indices);

			// BOTTOM.***
			scv->m_xRot = 90.0; scv->m_yRot = 0.0; scv->m_zRot = 0.0;
			drawAndDetectVisibleColorIndices(indices);
		}

		leafBox->meshes.clear();

		std::map<size_t, size_t>::iterator iter;
		for (iter = indices.begin(); iter != indices.end(); iter++)
		{
			if (iter->first >= meshCount)
				continue;

			leafBox->meshes.push_back(meshes[iter->first]);
		}

		scanningPoints.clear();
		indices.clear();
	}


	// Finally, return the normal perspective view.*******************************************************************
	scv->ClearColor[0] = 1.0;
	scv->ClearColor[1] = 1.0;
	scv->ClearColor[2] = 1.0;

	scv->m_xPos = currentXPos;
	scv->m_yPos = currentYPos;
	scv->m_zPos = currentZPos;

	scv->m_xRot = currentXRot;
	scv->m_yRot = currentYRot;
	scv->m_zRot = currentZRot;

	scv->m_xRot_aditional = 0.0;
	scv->m_yRot_aditional = 0.0;
	scv->m_zRot_aditional = 0.0;

	scv->m_perspective_angle = 90.0;
	defaultSpaceSetupForVisualization(scv->m_width, scv->m_height);
	setupPerspectiveViewSetting(fullBbox);

	//glReadBuffer(GL_BACK);
}

void ConversionProcessor::drawAndDetectVisibleColorIndices(std::map<size_t, size_t>& container)
{
	int pixelCount = OcclusionCullingScreenSize * OcclusionCullingScreenSize;
	GLubyte* buffer = new GLubyte[pixelCount * 3];
	memset(buffer, 0xff, sizeof(GLubyte) * pixelCount * 3);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Reset the model matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();

	glRotatef((float)scv->m_xRot_aditional, 1.0f, 0.0f, 0.0f);
	glRotatef((float)scv->m_yRot_aditional, 0.0f, 1.0f, 0.0f);
	glRotatef((float)scv->m_zRot_aditional, 0.0f, 0.0f, 1.0f);

	glRotatef((float)scv->m_xRot, 1.0f, 0.0f, 0.0f);
	glRotatef((float)scv->m_yRot, 0.0f, 1.0f, 0.0f);
	glRotatef((float)scv->m_zRot, 0.0f, 0.0f, 1.0f);

	// Apliquem les traslacions de l'escena.***
	glTranslatef((float)scv->m_xPos, (float)scv->m_yPos, (float)scv->m_zPos);

	//**********************************************************
			//if(this->m_one_displayList_id > 0)
	glCallList(DisplayListIdForOcclusionCulling);

	glPopMatrix();
	glFlush();

	glReadPixels(0, 0, OcclusionCullingScreenSize, OcclusionCullingScreenSize, GL_RGB, GL_UNSIGNED_BYTE, buffer);

	for (int j = 0; j < pixelCount; j++)
	{
		GLubyte pixel_data_R = buffer[j * 3];
		GLubyte pixel_data_G = buffer[j * 3 + 1];
		GLubyte pixel_data_B = buffer[j * 3 + 2];

		if (pixel_data_R == 255 && pixel_data_G == 255 && pixel_data_B == 255)
			continue;
		// Max color = (254, 254, 254).***
		// i= 64516*R + 254*G + B.***
		unsigned long index = MakeColorU4(pixel_data_R, pixel_data_G, pixel_data_B);
		container[index] = index;
	}

	delete[] buffer;
}

void ConversionProcessor::makeDisplayListOfMeshes(std::vector<gaia3d::TrianglePolyhedron*>& meshes, std::vector<gaia3d::ColorU4>& colors)
{
	glNewList(DisplayListIdForOcclusionCulling, GL_COMPILE);

	size_t meshCount = meshes.size();
	gaia3d::TrianglePolyhedron* mesh;
	for (size_t i = 0; i < meshCount; i++)
	{
		mesh = meshes[i];

		glColor3ub(GetRedValue(colors[i]), GetGreenValue(colors[i]), GetBlueValue(colors[i]));
		renderMesh(mesh, false, false);
	}
	glEndList();
}

void ConversionProcessor::renderMesh(gaia3d::TrianglePolyhedron* mesh, bool bNormal, bool bTextureCoordinate)
{
	char renderingMode = (bNormal ? 2 : 0) + (bTextureCoordinate ? 1 : 0);

	size_t surfaceCount = mesh->getSurfaces().size();
	gaia3d::Surface* surface;
	size_t triangleCount;
	gaia3d::Triangle* triangle;
	gaia3d::Vertex** vertices;

	switch (renderingMode)
	{
	case 0: // non normal, non texture coordinate
	{
		for (size_t i = 0; i < surfaceCount; i++)
		{
			surface = mesh->getSurfaces()[i];
			triangleCount = surface->getTriangles().size();

			glBegin(GL_TRIANGLES);
			for (size_t j = 0; j < triangleCount; j++)
			{
				triangle = surface->getTriangles()[j];
				vertices = triangle->getVertices();

				// 1rst vertex.***************************************************************
				glVertex3f((float)vertices[0]->position.x, (float)vertices[0]->position.y, (float)vertices[0]->position.z);

				// 2nd vertex.***************************************************************
				glVertex3f((float)vertices[1]->position.x, (float)vertices[1]->position.y, (float)vertices[1]->position.z);

				// 3rd vertex.***************************************************************
				glVertex3f((float)vertices[2]->position.x, (float)vertices[2]->position.y, (float)vertices[2]->position.z);
			}
			glEnd();
		}
	}
	break;
	case 1: // only texture coordiate
	{
		for (size_t i = 0; i < surfaceCount; i++)
		{
			surface = mesh->getSurfaces()[i];
			triangleCount = surface->getTriangles().size();

			glBegin(GL_TRIANGLES);
			for (size_t j = 0; j < triangleCount; j++)
			{
				triangle = surface->getTriangles()[j];
				vertices = triangle->getVertices();

				// 1rst vertex.***************************************************************
				glTexCoord2d(vertices[0]->textureCoordinate[0], vertices[0]->textureCoordinate[1]);
				glVertex3f((float)vertices[0]->position.x, (float)vertices[0]->position.y, (float)vertices[0]->position.z);

				// 2nd vertex.***************************************************************
				glTexCoord2d(vertices[1]->textureCoordinate[0], vertices[1]->textureCoordinate[1]);
				glVertex3f((float)vertices[1]->position.x, (float)vertices[1]->position.y, (float)vertices[1]->position.z);

				// 3rd vertex.***************************************************************
				glTexCoord2d(vertices[2]->textureCoordinate[0], vertices[2]->textureCoordinate[1]);
				glVertex3f((float)vertices[2]->position.x, (float)vertices[2]->position.y, (float)vertices[2]->position.z);
			}
			glEnd();
		}
	}
	break;
	case 2: // only normal
	{
		for (size_t i = 0; i < surfaceCount; i++)
		{
			surface = mesh->getSurfaces()[i];
			triangleCount = surface->getTriangles().size();

			glBegin(GL_TRIANGLES);
			for (size_t j = 0; j < triangleCount; j++)
			{
				triangle = surface->getTriangles()[j];
				vertices = triangle->getVertices();

				// 1rst vertex.***************************************************************
				glNormal3f((float)vertices[0]->normal.x, (float)vertices[0]->normal.y, (float)vertices[0]->normal.z);
				glVertex3f((float)vertices[0]->position.x, (float)vertices[0]->position.y, (float)vertices[0]->position.z);

				// 2nd vertex.***************************************************************
				glNormal3f((float)vertices[1]->normal.x, (float)vertices[1]->normal.y, (float)vertices[1]->normal.z);
				glVertex3f((float)vertices[1]->position.x, (float)vertices[1]->position.y, (float)vertices[1]->position.z);

				// 3rd vertex.***************************************************************
				glNormal3f((float)vertices[2]->normal.x, (float)vertices[2]->normal.y, (float)vertices[2]->normal.z);
				glVertex3f((float)vertices[2]->position.x, (float)vertices[2]->position.y, (float)vertices[2]->position.z);
			}
			glEnd();
		}
	}
	break;
	case 3: // both normal and texture coordinate
	{
		for (size_t i = 0; i < surfaceCount; i++)
		{
			surface = mesh->getSurfaces()[i];
			triangleCount = surface->getTriangles().size();

			glBegin(GL_TRIANGLES);
			for (size_t j = 0; j < triangleCount; j++)
			{
				triangle = surface->getTriangles()[j];
				vertices = triangle->getVertices();

				// 1rst vertex.***************************************************************
				glNormal3f((float)vertices[0]->normal.x, (float)vertices[0]->normal.y, (float)vertices[0]->normal.z);
				glTexCoord2d(vertices[0]->textureCoordinate[0], vertices[0]->textureCoordinate[1]);
				glVertex3f((float)vertices[0]->position.x, (float)vertices[0]->position.y, (float)vertices[0]->position.z);

				// 2nd vertex.***************************************************************
				glNormal3f((float)vertices[1]->normal.x, (float)vertices[1]->normal.y, (float)vertices[1]->normal.z);
				glTexCoord2d(vertices[1]->textureCoordinate[0], vertices[1]->textureCoordinate[1]);
				glVertex3f((float)vertices[1]->position.x, (float)vertices[1]->position.y, (float)vertices[1]->position.z);

				// 3rd vertex.***************************************************************
				glNormal3f((float)vertices[2]->normal.x, (float)vertices[2]->normal.y, (float)vertices[2]->normal.z);
				glTexCoord2d(vertices[2]->textureCoordinate[0], vertices[2]->textureCoordinate[1]);
				glVertex3f((float)vertices[2]->position.x, (float)vertices[2]->position.y, (float)vertices[2]->position.z);
			}
			glEnd();
		}
	}
	break;
	}
}

void ConversionProcessor::extractMatchedReferencesFromOcclusionInfo(gaia3d::VisionOctreeBox* receiver,
	gaia3d::VisionOctreeBox& info,
	std::vector<gaia3d::TrianglePolyhedron*>& meshesToBeCompared)
{
	// at this time, dimensions of receiver and info are same with each other.
	size_t childCount = receiver->children.size();
	if (childCount > 0)
	{
		for (size_t i = 0; i < childCount; i++)
		{
			extractMatchedReferencesFromOcclusionInfo(((gaia3d::VisionOctreeBox*)receiver->children[i]),
				*((gaia3d::VisionOctreeBox*)info.children[i]),
				meshesToBeCompared);
		}
	}
	else
	{
		receiver->meshes.clear();
		size_t meshCountInInfoBox = info.meshes.size();
		size_t meshCountInToBecompared = meshesToBeCompared.size();
		bool existsInToBeCompared;
		for (size_t i = 0; i < meshCountInInfoBox; i++)
		{
			existsInToBeCompared = false;
			for (size_t j = 0; j < meshCountInToBecompared; j++)
			{
				if (info.meshes[i] == meshesToBeCompared[j])
				{
					existsInToBeCompared = true;
					break;
				}
			}

			if (existsInToBeCompared)
				receiver->meshes.push_back(info.meshes[i]);
		}
	}
}

void ConversionProcessor::sortTrianglesBySize(std::vector<gaia3d::Triangle*>& inputTriangles,
	unsigned char sizeLevels,
	double* sizeArray,
	std::vector<gaia3d::Triangle*>& outputTriangles,
	unsigned int* sizeIndexMarkers)
{

	double* squaredSizeArray = new double[sizeLevels];
	for (unsigned char i = 0; i < sizeLevels; i++)
		squaredSizeArray[i] = sizeArray[i] * sizeArray[i];

	std::vector<std::vector<gaia3d::Triangle*>*> eachSizeTriangles;
	for (unsigned char i = 0; i <= sizeLevels; i++)
		eachSizeTriangles.push_back(new std::vector<gaia3d::Triangle*>);

	size_t triangleCount = inputTriangles.size();
	double diffX, diffY, diffZ;
	double squaredEdgeLength0, squaredEdgeLength1, squaredEdgeLength2, minSquaredEdgeLength;
	for (size_t i = 0; i < triangleCount; i++)
	{
		diffX = inputTriangles[i]->getVertices()[0]->position.x - inputTriangles[i]->getVertices()[1]->position.x;
		diffY = inputTriangles[i]->getVertices()[0]->position.y - inputTriangles[i]->getVertices()[1]->position.y;
		diffZ = inputTriangles[i]->getVertices()[0]->position.z - inputTriangles[i]->getVertices()[1]->position.z;
		squaredEdgeLength0 = diffX * diffX + diffY * diffY + diffZ * diffZ;

		diffX = inputTriangles[i]->getVertices()[2]->position.x - inputTriangles[i]->getVertices()[1]->position.x;
		diffY = inputTriangles[i]->getVertices()[2]->position.y - inputTriangles[i]->getVertices()[1]->position.y;
		diffZ = inputTriangles[i]->getVertices()[2]->position.z - inputTriangles[i]->getVertices()[1]->position.z;
		squaredEdgeLength1 = diffX * diffX + diffY * diffY + diffZ * diffZ;

		diffX = inputTriangles[i]->getVertices()[2]->position.x - inputTriangles[i]->getVertices()[0]->position.x;
		diffY = inputTriangles[i]->getVertices()[2]->position.y - inputTriangles[i]->getVertices()[0]->position.y;
		diffZ = inputTriangles[i]->getVertices()[2]->position.z - inputTriangles[i]->getVertices()[0]->position.z;
		squaredEdgeLength2 = diffX * diffX + diffY * diffY + diffZ * diffZ;

		minSquaredEdgeLength = (squaredEdgeLength0 > squaredEdgeLength1) ?
			((squaredEdgeLength1 > squaredEdgeLength2) ? squaredEdgeLength2 : squaredEdgeLength1) :
			((squaredEdgeLength0 > squaredEdgeLength2) ? squaredEdgeLength2 : squaredEdgeLength0);

		bool bSorted = false;
		for (unsigned char j = 0; j < sizeLevels; j++)
		{
			if (minSquaredEdgeLength > squaredSizeArray[j])
			{
				eachSizeTriangles[j]->push_back(inputTriangles[i]);
				bSorted = true;
				break;
			}
		}
		if (!bSorted)
			eachSizeTriangles[sizeLevels]->push_back(inputTriangles[i]);
	}

	for (unsigned char i = 0; i < sizeLevels; i++)
	{
		outputTriangles.insert(outputTriangles.end(), eachSizeTriangles[i]->begin(), eachSizeTriangles[i]->end());
		sizeIndexMarkers[i] = (unsigned int)outputTriangles.size();
		delete eachSizeTriangles[i];
	}
	outputTriangles.insert(outputTriangles.end(), eachSizeTriangles[sizeLevels]->begin(), eachSizeTriangles[sizeLevels]->end());
	delete eachSizeTriangles[sizeLevels];

	delete[] squaredSizeArray;
}

void ConversionProcessor::loadAndBindTextures(std::map<std::string, unsigned char*>& textures,
	std::map<std::string, unsigned int>& textureWidths,
	std::map<std::string, unsigned int>& textureHeights,
	std::map<std::string, unsigned int>& bindingResult)
{
	std::map<std::string, unsigned char*>::iterator itr = textures.begin();
	std::string fileName;
	for (; itr != textures.end(); itr++)
	{
		fileName = itr->first;
		unsigned char* textureData = itr->second;
		int width = textureWidths[fileName];
		int height = textureHeights[fileName];
		unsigned int idTextureBound = 0;

		// binding
		glGenTextures(1, &idTextureBound);
		glBindTexture(GL_TEXTURE_2D, idTextureBound);

		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
		glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);

		bindingResult.insert(std::map<std::string, unsigned int>::value_type(fileName, idTextureBound));
	}
}

void ConversionProcessor::drawMeshesWithTextures(std::vector<gaia3d::TrianglePolyhedron*>& meshes, std::map<std::string, unsigned int>& bindingResult, unsigned int shaderProgram)
{
	// do this only once.
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Shader.
	int oneVertexBuffSize = 3 * 4 + 3 * 4 + 2 * 4;
	GLfloat mv_matrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, mv_matrix);

	GLfloat proj_matrix[16];
	glGetFloatv(GL_PROJECTION_MATRIX, proj_matrix);

	GLfloat mvProj_matrix[16];

	gaia3d::Matrix4 mv_mat, proj_mat, mvProj_mat, mv_invMat, mv_invTransposedMat;

	mv_mat.set(mv_matrix[0], mv_matrix[4], mv_matrix[8], mv_matrix[12],
		mv_matrix[1], mv_matrix[5], mv_matrix[9], mv_matrix[13],
		mv_matrix[2], mv_matrix[6], mv_matrix[10], mv_matrix[14],
		mv_matrix[3], mv_matrix[7], mv_matrix[11], mv_matrix[15]);

	proj_mat.set(proj_matrix[0], proj_matrix[4], proj_matrix[8], proj_matrix[12],
		proj_matrix[1], proj_matrix[5], proj_matrix[9], proj_matrix[13],
		proj_matrix[2], proj_matrix[6], proj_matrix[10], proj_matrix[14],
		proj_matrix[3], proj_matrix[7], proj_matrix[11], proj_matrix[15]);

	mvProj_mat = proj_mat * mv_mat;
	mvProj_mat.getFloatArray(mvProj_matrix);

	mv_invMat = mv_mat.inverse();
	mv_invTransposedMat = mv_invMat.transpose();
	float normalMat3[9];
	mv_invMat.getOnlyRotationFloatArray(normalMat3);

	glUseProgram(shaderProgram);

	int normalMatrix_location = glGetUniformLocation(shaderProgram, "uNMatrix");
	int mvMatrix_location = glGetUniformLocation(shaderProgram, "ModelViewProjectionMatrix");

	glUniformMatrix4fv(mvMatrix_location, 1, false, mvProj_matrix);
	glUniformMatrix3fv(normalMatrix_location, 1, false, normalMat3);

	int position_location = glGetAttribLocation(shaderProgram, "position");
	int texCoord_location = glGetAttribLocation(shaderProgram, "aTextureCoord");
	//int normal_location = glGetAttribLocation(shaderProgram, "aVertexNormal");

	glEnableVertexAttribArray(position_location);
	glEnableVertexAttribArray(texCoord_location);

	int samplerUniform = glGetUniformLocation(shaderProgram, "uSampler");
	glUniform1i(samplerUniform, 0);

	int hasTexture_location = glGetUniformLocation(shaderProgram, "hasTexture");
	int colorAux_location = glGetUniformLocation(shaderProgram, "vColorAux");
	//int bUseLighting_location = glGetUniformLocation(shaderProgram, "useLighting");

	//glUniform1i(bUseLighting_location, false);

	//glUniform1i(hasTexture_location, true);

	size_t meshCount = meshes.size();
	for (size_t i = 0; i < meshCount; i++)
	{
		gaia3d::TrianglePolyhedron* polyhedron = meshes[i];

		size_t vboCount = polyhedron->getVbos().size();
		// 이럴 일은 없겠지만 vbo가 없다는 것은 실체가 없는 객체이므로
		// filtering 한다.
		if (vboCount == 0)
			continue;

		GLfloat* vertices, * textureCoordinates;
		GLushort* indices;
		unsigned int textureCoordinateArrayId;
		for (size_t j = 0; j < vboCount; j++)
		{
			vertices = textureCoordinates = NULL;
			indices = NULL;

			gaia3d::Vbo* vbo = polyhedron->getVbos()[j];

			// make vertex array
			size_t vertexCount = vbo->vertices.size();
			if (vertexCount == 0)
				continue;

			vertices = new GLfloat[vertexCount * 3];
			memset(vertices, 0x00, sizeof(GLfloat) * 3 * vertexCount);

			for (size_t k = 0; k < vertexCount; k++)
			{
				vertices[k * 3] = float(vbo->vertices[k]->position.x);
				vertices[k * 3 + 1] = float(vbo->vertices[k]->position.y);
				vertices[k * 3 + 2] = float(vbo->vertices[k]->position.z);
			}

			// make index array
			size_t indexCount = vbo->indices.size();
			indices = new GLushort[indexCount];
			memset(indices, 0x00, sizeof(GLushort) * indexCount);
			for (size_t k = 0; k < indexCount; k++)
				indices[k] = vbo->indices[k];

			// at this point, all necessary arrays are made

			// bind vertex array
			unsigned int vertexArrayId;
			glGenBuffers(1, &vertexArrayId);
			glBindBuffer(GL_ARRAY_BUFFER, vertexArrayId);
			glBufferData(GL_ARRAY_BUFFER, vertexCount * 3 * 4, vertices, GL_STATIC_DRAW);
			glVertexAttribPointer(position_location, 3, GL_FLOAT, GL_FALSE, 3 * 4, (void*)0);    //send positions on pipe 0
			delete[] vertices;

			// bind index array
			unsigned int indexArrayId;
			glGenBuffers(1, &indexArrayId);// generate a new VBO and get the associated ID
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexArrayId);// bind VBO in order to use
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * 2, indices, GL_STATIC_DRAW);// upload data to VBO
			delete[] indices;

			if (polyhedron->doesThisHaveTextureCoordinates())
			{
				glActiveTexture(GL_TEXTURE0 + 0);
				// 이 polyhedron을 그릴 때 필요한 texture를 activate 시킨다.
				std::string textureName = polyhedron->getStringAttribute(std::string(TextureName));
				if (bindingResult.find(textureName) != bindingResult.end())
				{
					unsigned int textureId = bindingResult[textureName];
					glBindTexture(GL_TEXTURE_2D, textureId);
				}

				glUniform1i(hasTexture_location, true);

				// make texture coordinate array
				textureCoordinates = new GLfloat[vertexCount * 2];
				memset(textureCoordinates, 0x00, sizeof(GLfloat) * 2 * vertexCount);
				for (size_t k = 0; k < vertexCount; k++)
				{
					textureCoordinates[k * 2] = float(vbo->vertices[k]->textureCoordinate[0]);
					textureCoordinates[k * 2 + 1] = float(vbo->vertices[k]->textureCoordinate[1]);
				}

				// bind texture coordinate array
				glGenBuffers(1, &textureCoordinateArrayId);
				glBindBuffer(GL_ARRAY_BUFFER, textureCoordinateArrayId);
				glBufferData(GL_ARRAY_BUFFER, vertexCount * 2 * 4, textureCoordinates, GL_STATIC_DRAW);
				glVertexAttribPointer(texCoord_location, 2, GL_FLOAT, GL_FALSE, 2 * 4, (void*)0);
				delete[] textureCoordinates;
			}
			else
			{
				glUniform1i(hasTexture_location, false);
				glDisableVertexAttribArray(texCoord_location);
				float colorR, colorG, colorB;

				if (polyhedron->getColorMode() == gaia3d::SingleColor)
				{
					colorR = GetRedValue(polyhedron->getSingleColor()) / 255.0f;
					colorG = GetGreenValue(polyhedron->getSingleColor()) / 255.0f;
					colorB = GetBlueValue(polyhedron->getSingleColor()) / 255.0f;
				}
				else
				{
					colorR = GetRedValue(polyhedron->getVertices()[0]->color) / 255.0f;
					colorG = GetGreenValue(polyhedron->getVertices()[0]->color) / 255.0f;
					colorB = GetBlueValue(polyhedron->getVertices()[0]->color) / 255.0f;
				}

				glUniform3f(colorAux_location, colorR, colorG, colorB);
			}

			glDrawElements(GL_TRIANGLES, (int)indexCount, GL_UNSIGNED_SHORT, 0);

			glDeleteBuffers(1, &vertexArrayId);
			glDeleteBuffers(1, &indexArrayId);
			if (polyhedron->doesThisHaveTextureCoordinates())
				glDeleteBuffers(1, &textureCoordinateArrayId);
		}
	}

	glDisableVertexAttribArray(position_location);
	glDisableVertexAttribArray(texCoord_location);

	glUseProgram(0);
}

void ConversionProcessor::unbindTextures(std::map<std::string, unsigned int>& bindingResult)
{
	std::map<std::string, unsigned int>::iterator itr;
	for (itr = bindingResult.begin(); itr != bindingResult.end(); itr++)
	{
		unsigned int textureId = itr->second;
		glDeleteTextures(1, &textureId);
	}

	bindingResult.clear();
}

unsigned int ConversionProcessor::makeShaders()
{
	// vertex shader source
	GLchar vs_source[] =
	{
		"#version 120\n"
		"attribute vec3 position;\n"
		"attribute vec4 aVertexColor;\n"
		"attribute vec2 aTextureCoord;\n"
		"uniform mat4 ModelViewProjectionMatrix;\n"
		"varying vec4 vColor;\n"
		"varying vec2 vTextureCoord;\n"
		"attribute vec3 aVertexNormal;\n"
		"varying vec3 uAmbientColor;\n"
		"varying vec3 vLightWeighting;\n"
		"uniform vec3 uLightingDirection;\n"
		"uniform mat3 uNMatrix;\n"
		"void main(void) {\n"
		"vec4 pos = vec4(position.xyz, 1.0);\n"
		"gl_Position = ModelViewProjectionMatrix * pos;\n"
		"vColor = aVertexColor;\n"
		"vTextureCoord = aTextureCoord;\n"
		"\n"
		"vLightWeighting = vec3(1.0, 1.0, 1.0);\n"
		"uAmbientColor = vec3(0.6, 0.6, 0.6);\n"
		"vec3 uLightingDirection = vec3(0.2, 0.2, -0.9);\n"
		"vec3 directionalLightColor = vec3(0.6, 0.6, 0.6);\n"
		"vec3 transformedNormal = uNMatrix * aVertexNormal;\n"
		"float directionalLightWeighting = max(dot(transformedNormal, uLightingDirection), 0.0);\n"
		"vLightWeighting = uAmbientColor + directionalLightColor * directionalLightWeighting;\n"
		"}\n"
	};

	const GLchar* vertexShaderSource = vs_source;

	// create vertex shader object
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	// attatch & compile vertex shader source
	glShaderSource(vertexShader, 1, &vertexShaderSource, 0);
	glCompileShader(vertexShader);
	GLint isCompiled = 0;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<char> errorLog(maxLength);
		glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &errorLog[0]);

		std::cout << "vertexShader failed to compile" << std::endl;

		for (std::vector<char>::const_iterator i = errorLog.begin(); i != errorLog.end(); ++i)
		{
			std::cout << *i;
		}
		std::cout << std::endl;

		glDeleteShader(vertexShader);
		return 0;
	}

	// fragment shader source
	GLchar fs_source[] =
	{
		"#version 120\n"
		"varying vec4 vColor;\n"
		"varying vec2 vTextureCoord;\n"
		"uniform sampler2D uSampler;\n"
		"uniform bool hasTexture;\n"
		"uniform bool useLighting;\n"
		"uniform vec3 vColorAux;\n"
		"varying vec3 vLightWeighting;\n"
		"void main(void) {\n"
		"if(hasTexture)\n"
		"{\n"
		"vec4 textureColor = texture2D(uSampler, vec2(vTextureCoord.s, vTextureCoord.t));\n"
		"if(useLighting)\n"
		"{\n"
		"gl_FragColor = vec4(textureColor.rgb * vLightWeighting, textureColor.a);\n"
		"}\n"
		"else\n"
		"{\n"
		"gl_FragColor = textureColor;\n"
		"}\n"
		"}\n"
		"else \n"
		"{\n"
		"vec4 textureColor = vec4(vColorAux.xyz, 1.0);\n"
		"if(useLighting)\n"
		"{\n"
		"gl_FragColor = vec4(textureColor.rgb * vLightWeighting, textureColor.a);\n"
		"}\n"
		"else\n"
		"{\n"
		"gl_FragColor = textureColor;\n"
		"}\n"
		"}\n"
		"}\n"
	};

	const GLchar* fragmentShaderSource = fs_source;

	// create fragment shader object
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// attatch & compile fragment shader source
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, 0);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<char> errorLog(maxLength);
		glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, &errorLog[0]);

		std::cout << "fragmentShader failed to compile" << std::endl;

		for (std::vector<char>::const_iterator i = errorLog.begin(); i != errorLog.end(); ++i)
		{
			std::cout << *i;
		}
		std::cout << std::endl;

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return 0;
	}

	//Vertex and fragment shaders are successfully compiled.
	//Now time to link them together into a program.
	//Get a program object.
	GLuint program = glCreateProgram();

	//Attach our shaders to our program
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	//Link our program
	glLinkProgram(program);
	GLint isLinked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
	if (isLinked == GL_FALSE)
	{
		glDeleteProgram(program);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return 0;
	}

	//Always detach shaders after a successful link.
	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
}

unsigned int ConversionProcessor::makeShadersForNSM()
{
	// vertex shader source
	GLchar vs_source[] =
	{
		"#version 120\n"
		"attribute vec3 position;\n"
		"attribute vec4 aVertexColor;\n"
		"attribute vec2 aTextureCoord;\n"
		"uniform mat4 ModelViewProjectionMatrix;\n"
		"varying vec4 vColor;\n"
		"varying vec2 vTextureCoord;\n"
		"attribute vec3 aVertexNormal;\n"
		"varying vec3 uAmbientColor;\n"
		"varying vec3 vLightWeighting;\n"
		"uniform vec3 uLightingDirection;\n"
		"uniform mat3 uNMatrix;\n"
		"void main(void) {\n"
		"vec4 pos = vec4(position.xyz, 1.0);\n"
		"gl_Position = ModelViewProjectionMatrix * pos;\n"
		"vColor = aVertexColor;\n"
		"vTextureCoord = aTextureCoord;\n"
		"\n"
		"vLightWeighting = vec3(1.0, 1.0, 1.0);\n"
		"uAmbientColor = vec3(0.6, 0.6, 0.6);\n"
		"vec3 uLightingDirection = vec3(0.2, 0.2, -0.9);\n"
		"vec3 directionalLightColor = vec3(0.6, 0.6, 0.6);\n"
		"vec3 transformedNormal = uNMatrix * aVertexNormal;\n"
		"float directionalLightWeighting = max(dot(transformedNormal, uLightingDirection), 0.0);\n"
		"vLightWeighting = uAmbientColor + directionalLightColor * directionalLightWeighting;\n"
		"}\n"
	};

	const GLchar* vertexShaderSource = vs_source;

	// create vertex shader object
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	// attatch & compile vertex shader source
	glShaderSource(vertexShader, 1, &vertexShaderSource, 0);
	glCompileShader(vertexShader);
	GLint isCompiled = 0;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<char> errorLog(maxLength);
		glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &errorLog[0]);

		std::cout << "vertexShader failed to compile" << std::endl;

		for (std::vector<char>::const_iterator i = errorLog.begin(); i != errorLog.end(); ++i)
		{
			std::cout << *i;
		}
		std::cout << std::endl;

		glDeleteShader(vertexShader);
		return 0;
	}

	// fragment shader source
	GLchar fs_source[] =
	{
		"#version 120\n"
		"varying vec4 vColor;\n"
		"varying vec2 vTextureCoord;\n"
		"uniform sampler2D uSampler;\n"
		"uniform bool hasTexture;\n"
		"uniform bool useLighting;\n"
		"uniform vec3 vColorAux;\n"
		"varying vec3 vLightWeighting;\n"
		"void main(void) {\n"
		"if(hasTexture)\n"
		"{\n"
		"vec4 textureColor = texture2D(uSampler, vec2(vTextureCoord.s, vTextureCoord.t));\n"
		"if(useLighting)\n"
		"{\n"
		"if(textureColor.r > 0.97 && textureColor.g > 0.97 && textureColor.b > 0.97)\n"
		"{\n"
		"textureColor.r = 0.95;\n"
		"textureColor.g = 0.95;\n"
		"textureColor.b = 0.95;\n"
		"}\n"
		"textureColor.a = 1.0;\n"
		"gl_FragColor = vec4(textureColor.rgb * vLightWeighting, textureColor.a);\n"
		"}\n"
		"else\n"
		"{\n"
		"if(textureColor.r > 0.97 && textureColor.g > 0.97 && textureColor.b > 0.97)\n"
		"{\n"
		"textureColor.r = 0.95;\n"
		"textureColor.g = 0.95;\n"
		"textureColor.b = 0.95;\n"
		"}\n"
		"textureColor.a = 1.0;\n"
		"gl_FragColor = textureColor;\n"
		"}\n"
		"}\n"
		"else \n"
		"{\n"
		"vec4 textureColor = vec4(vColorAux.xyz, 1.0);\n"
		"if(useLighting)\n"
		"{\n"
		"if(textureColor.r > 0.97 && textureColor.g > 0.97 && textureColor.b > 0.97)\n"
		"{\n"
		"textureColor.r = 0.95;\n"
		"textureColor.g = 0.95;\n"
		"textureColor.b = 0.95;\n"
		"}\n"
		"textureColor.a = 1.0;\n"
		"gl_FragColor = vec4(textureColor.rgb * vLightWeighting, textureColor.a);\n"
		"}\n"
		"else\n"
		"{\n"
		"if(textureColor.r > 0.97 && textureColor.g > 0.97 && textureColor.b > 0.97)\n"
		"{\n"
		"textureColor.r = 0.95;\n"
		"textureColor.g = 0.95;\n"
		"textureColor.b = 0.95;\n"
		"}\n"
		"textureColor.a = 1.0;\n"
		"gl_FragColor = textureColor;\n"
		"}\n"
		"}\n"
		"}\n"
	};

	const GLchar* fragmentShaderSource = fs_source;

	// create fragment shader object
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// attatch & compile fragment shader source
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, 0);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<char> errorLog(maxLength);
		glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, &errorLog[0]);

		std::cout << "fragmentShader failed to compile" << std::endl;

		for (std::vector<char>::const_iterator i = errorLog.begin(); i != errorLog.end(); ++i)
		{
			std::cout << *i;
		}
		std::cout << std::endl;

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return 0;
	}

	//Vertex and fragment shaders are successfully compiled.
	//Now time to link them together into a program.
	//Get a program object.
	GLuint program = glCreateProgram();

	//Attach our shaders to our program
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	//Link our program
	glLinkProgram(program);
	GLint isLinked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
	if (isLinked == GL_FALSE)
	{
		glDeleteProgram(program);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return 0;
	}

	//Always detach shaders after a successful link.
	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
}

void ConversionProcessor::deleteShaders(unsigned int programId)
{
	glUseProgram(0);
	glDeleteProgram(programId);
}

void ConversionProcessor::makeNetSurfaceMeshes(gaia3d::SpatialOctreeBox& octrees,
	std::map<std::string, unsigned char*>& textures,
	std::map<std::string, unsigned int>& textureWidths,
	std::map<std::string, unsigned int>& textureHeights,
	std::map<unsigned char, unsigned char>& lodUsingOriginalMesh)
{
	// prepare shaders for depth detection and texture drawing
	unsigned int shaderProgramDepthDetection = makeShadersForNSM();
	if (shaderProgramDepthDetection == 0)
		return;

	unsigned int shaderProgramTexture = makeShaders();
	if (shaderProgramTexture == 0)
	{
		deleteShaders(shaderProgramDepthDetection);
		return;
	}

	// load and bind textures
	std::map<std::string, unsigned int> bindingResult;
	loadAndBindTextures(textures, textureWidths, textureHeights, bindingResult);

	// fill each leaf octree with NSM of lod 2~N
	std::vector<gaia3d::OctreeBox*> container;
	octrees.getAllLeafBoxes(container, true);
	for (unsigned char i = 2; i <= MaxLod; i++)
	{
		NetSurfaceMeshMaker maker;
		bool bUseOriginalMesh = lodUsingOriginalMesh.find(i) == lodUsingOriginalMesh.end() ? false : true;
		if (bUseOriginalMesh)
			maker.makeNetSurfaceMesh(container,
				settings.nsmSettings[i],
				netSurfaceMeshes);
		else
			maker.makeNetSurfaceMesh(container,
				settings.nsmSettings[i],
				this->scv,
				shaderProgramDepthDetection,
				shaderProgramTexture,
				bindingResult,
				netSurfaceMeshes,
				netSurfaceTextures,
				netSurfaceTextureWidth,
				netSurfaceTextureHeight);
	}


	// delete shaders
	deleteShaders(shaderProgramDepthDetection);
	deleteShaders(shaderProgramTexture);

	// unbind textures
	unbindTextures(bindingResult);

	// restore rendering environment
	defaultSpaceSetupForVisualization(scv->m_width, scv->m_height);

	// make bounding box of result polyhedrons
	size_t octreeCount = container.size();
	for (size_t i = 0; i < octreeCount; i++)
		if (((gaia3d::SpatialOctreeBox*)container[i])->netSurfaceMesh != NULL)
			calculateBoundingBox(((gaia3d::SpatialOctreeBox*)container[i])->netSurfaceMesh);

	std::map<unsigned char, gaia3d::TrianglePolyhedron*>::iterator iter = netSurfaceMeshes.begin();
	for (; iter != netSurfaceMeshes.end(); iter++)
		calculateBoundingBox(iter->second);

	// normalize mosaic textures
	normalizeMosiacTextures(netSurfaceTextures, netSurfaceTextureWidth, netSurfaceTextureHeight);
}

void ConversionProcessor::makeSkinMeshes(gaia3d::BoundingBox& bbox,
	std::vector<gaia3d::TrianglePolyhedron*>& meshes,
	gaia3d::SpatialOctreeBox& octree,
	std::map<std::string, unsigned char*>& textures,
	std::map<std::string, unsigned int>& textureWidths,
	std::map<std::string, unsigned int>& textureHeights)
{
	// extract and copy exterior triangles
	std::vector<gaia3d::Triangle*> triangles;
	extractExteriorTriangles(meshes, triangles, true);

	if (triangles.empty())
		return;

	// distribute collected triangles into each leaf octrees
	// and make pretty skin mesh for each octree
	octree.makeSkinMesh(triangles);
	triangles.clear();

	// merge skin meshes in each octree into single one for rougher lod
	std::vector<gaia3d::OctreeBox*> container;
	octree.getAllLeafBoxes(container, true);
	for (size_t i = 0; i < container.size(); i++)
	{
		gaia3d::SpatialOctreeBox* leafOctree = (gaia3d::SpatialOctreeBox *)container[i];
		gaia3d::TrianglePolyhedron* unitSkinMesh = leafOctree->prettySkinMesh;

		if (unitSkinMesh == NULL)
			continue;
		
		for (size_t j = 0; j < unitSkinMesh->getSurfaces().size(); j++)
		{
			gaia3d::Surface* surface = unitSkinMesh->getSurfaces()[j];
			for (size_t k = 0; k < surface->getTriangles().size(); k++)
				triangles.push_back(surface->getTriangles()[k]);
		}
	}

	// make textures for skin mesh in each octree and thumbnail
	makeSkinTexturesAndThumbnail(bbox, meshes, octree, textures, textureWidths, textureHeights);

	normalizeMosiacTextures(skinMeshTextures, skinMeshTextureWidth, skinMeshTextureHeight);

	// merge pretty lod meshes in each leaf octree into single one for lod 3 or higher
	gaia3d::TrianglePolyhedron* mergedSkinMesh = gaia3d::GeometryUtility::makeSingleMeshWithTriangles(triangles);
	calculateBoundingBox(mergedSkinMesh);
	mergedSkinMesh->setHasNormals(((gaia3d::SpatialOctreeBox*)container[0])->prettySkinMesh->doesThisHaveNormals());
	mergedSkinMesh->setHasTextureCoordinates(((gaia3d::SpatialOctreeBox*)container[0])->prettySkinMesh->doesThisHaveTextureCoordinates());
	skinMeshes[3] = mergedSkinMesh;
}

void ConversionProcessor::makeSkinTexturesAndThumbnail(
	gaia3d::BoundingBox& bbox,
	std::vector<gaia3d::TrianglePolyhedron*>& meshes,
	gaia3d::SpatialOctreeBox& octree,
	std::map<std::string, unsigned char*>& textures,
	std::map<std::string, unsigned int>& textureWidths,
	std::map<std::string, unsigned int>& textureHeights)
{
	// make texstures for pretty skin mesh(LOD2 and LOD3)
	// also make textures for thumbnail

	std::vector<gaia3d::OctreeBox*> container;
	octree.getAllLeafBoxes(container, true);
	if (container.empty())
		return;

	// make shader for drawing meshes
	unsigned int shaderProgramTexture = makeShaders();
	if (shaderProgramTexture == 0)
		return;

	// load and bind textures
	std::map<std::string, unsigned int> bindingResult;
	loadAndBindTextures(textures, textureWidths, textureHeights, bindingResult);

	// make 6 textures on each leaf octree box(top, bottom, front, rear, left, right)
	std::map<gaia3d::SpatialOctreeBox*, std::vector<unsigned char*>> sixTexturesOnEachBox;
	unsigned int imageWidth = TextureWidthForSkinMesh, imageHeight = TextureHeightForSkinMesh;
	for (size_t i = 0; i < container.size(); i++)
	{
		gaia3d::SpatialOctreeBox* leafOctree = (gaia3d::SpatialOctreeBox*)container[i];
		gaia3d::BoundingBox bbox;
		bbox.addPoint(leafOctree->minX, leafOctree->minY, leafOctree->minZ);
		bbox.addPoint(leafOctree->maxX, leafOctree->maxY, leafOctree->maxZ);
		std::vector<unsigned char*> output;
		makeSixFaceTexturesOnBox(leafOctree->meshes, bbox, output, imageWidth, imageHeight, shaderProgramTexture, bindingResult);
		sixTexturesOnEachBox[leafOctree] = output;
	}

	// merge each octree's texsture into single texture and make texture coordinates of skin mesh
	unsigned char* mosaicTexture = NULL;
	unsigned int mosaicWidth = 0, mosaicHeight = 0;
	mergeFaceTexsturesIntoSingleOne(sixTexturesOnEachBox, imageWidth, imageHeight, mosaicTexture, mosaicWidth, mosaicHeight);
	skinMeshTextures[2] = mosaicTexture;
	skinMeshTextureWidth[2] = mosaicWidth;
	skinMeshTextureHeight[2] = mosaicHeight;
	mosaicTexture = new unsigned char[mosaicWidth * mosaicHeight * 4];
	memcpy(mosaicTexture, skinMeshTextures[2], sizeof(unsigned char) * mosaicWidth * mosaicHeight * 4);
	skinMeshTextures[3] = mosaicTexture;
	skinMeshTextureWidth[3] = mosaicWidth;
	skinMeshTextureHeight[3] = mosaicHeight;
	mosaicTexture = new unsigned char[mosaicWidth * mosaicHeight * 4];
	memcpy(mosaicTexture, skinMeshTextures[2], sizeof(unsigned char) * mosaicWidth * mosaicHeight * 4);
	skinMeshTextures[4] = mosaicTexture;
	skinMeshTextureWidth[4] = mosaicWidth;
	skinMeshTextureHeight[4] = mosaicHeight;
	mosaicTexture = new unsigned char[mosaicWidth * mosaicHeight * 4];
	memcpy(mosaicTexture, skinMeshTextures[2], sizeof(unsigned char) * mosaicWidth * mosaicHeight * 4);
	skinMeshTextures[5] = mosaicTexture;
	skinMeshTextureWidth[5] = mosaicWidth;
	skinMeshTextureHeight[5] = mosaicHeight;

	std::map<gaia3d::SpatialOctreeBox*, std::vector<unsigned char*>>::iterator iterFaceTextures = sixTexturesOnEachBox.begin();
	for (; iterFaceTextures != sixTexturesOnEachBox.end(); iterFaceTextures++)
	{
		size_t textureCount = (iterFaceTextures->second).size();
		for (size_t i = 0; i < textureCount; i++)
			delete[] (iterFaceTextures->second)[i];

		(iterFaceTextures->second).clear();
	}
	sixTexturesOnEachBox.clear();

	// make thumbnail
	thumbnail =  makeFaceTextureOnBox(2, meshes, bbox, ThumbnailImageWidthHeight, ThumbnailImageWidthHeight, shaderProgramTexture, bindingResult);
	thumbnailWidth = thumbnailHeight = ThumbnailImageWidthHeight;

	// delete shader
	deleteShaders(shaderProgramTexture);

	// unbind textures
	unbindTextures(bindingResult);

	// restore rendering environment
	defaultSpaceSetupForVisualization(scv->m_width, scv->m_height);
}

void ConversionProcessor::extractExteriorTriangles(
	std::vector<gaia3d::TrianglePolyhedron*>& meshes,
	std::vector<gaia3d::Triangle*>& outputTriangles,
	bool bCopy)
{
	// collect exterior surfaces
	std::vector<gaia3d::Surface*> surfaces;
	for (size_t i = 0; i < meshes.size(); i++)
	{
		gaia3d::TrianglePolyhedron* mesh = meshes[i];
		size_t surfaceCount = mesh->getSurfaces().size();
		for (size_t j = 0; j < surfaceCount; j++)
		{
			gaia3d::Surface* surface = mesh->getSurfaces()[j];
			if (surface->isExterior())
				surfaces.push_back(surface);
		}
	}

	// collect exterior triangles
	std::vector<gaia3d::Triangle*> triangles;
	double sizeThreshold = 0.5;
	for (size_t i = 0; i < surfaces.size(); i++)
	{
		gaia3d::BoundingBox bbox;
		size_t triangleCount = surfaces[i]->getTriangles().size();
		for (size_t j = 0; j < triangleCount; j++)
		{
			gaia3d::Triangle* triangle = surfaces[i]->getTriangles()[j];
			gaia3d::Vertex** vertices = triangle->getVertices();
			bbox.addPoint(vertices[0]->position.x, vertices[0]->position.y, vertices[0]->position.z);
			bbox.addPoint(vertices[1]->position.x, vertices[1]->position.y, vertices[1]->position.z);
			bbox.addPoint(vertices[2]->position.x, vertices[2]->position.y, vertices[2]->position.z);
		}

		if (bbox.getMaxLength() < sizeThreshold)
			continue;

		if (bCopy)
		{
			for (size_t j = 0; j < triangleCount; j++)
			{
				gaia3d::Triangle* triangle = surfaces[i]->getTriangles()[j];
				if (triangle->isExterior())
				{
					gaia3d::Vertex* copiedVertex0 = new gaia3d::Vertex;
					copiedVertex0->position = triangle->getVertices()[0]->position;
					copiedVertex0->normal = triangle->getVertices()[0]->normal;
					copiedVertex0->color = triangle->getVertices()[0]->color;
					memcpy(copiedVertex0->textureCoordinate, triangle->getVertices()[0]->textureCoordinate, sizeof(double) * 2);
					gaia3d::Vertex* copiedVertex1 = new gaia3d::Vertex;
					copiedVertex1->position = triangle->getVertices()[1]->position;
					copiedVertex1->normal = triangle->getVertices()[1]->normal;
					copiedVertex1->color = triangle->getVertices()[1]->color;
					memcpy(copiedVertex1->textureCoordinate, triangle->getVertices()[1]->textureCoordinate, sizeof(double) * 2);
					gaia3d::Vertex* copiedVertex2 = new gaia3d::Vertex;
					copiedVertex2->position = triangle->getVertices()[2]->position;
					copiedVertex2->normal = triangle->getVertices()[2]->normal;
					copiedVertex2->color = triangle->getVertices()[2]->color;
					memcpy(copiedVertex2->textureCoordinate, triangle->getVertices()[2]->textureCoordinate, sizeof(double) * 2);

					gaia3d::Triangle* copiedTriangle = new gaia3d::Triangle;
					copiedTriangle->setVertices(copiedVertex0, copiedVertex1, copiedVertex2);
					copiedTriangle->setIfExterior(true);
					copiedTriangle->setNormal(triangle->getNormal()->x, triangle->getNormal()->y, triangle->getNormal()->z);
					outputTriangles.push_back(copiedTriangle);
				}
			}
		}
		else
		{
			for (size_t j = 0; j < triangleCount; j++)
			{
				gaia3d::Triangle* triangle = surfaces[i]->getTriangles()[j];
				if (triangle->isExterior())
					outputTriangles.push_back(triangle);
			}
		}
	}
}

void ConversionProcessor::normalizeMosiacTextures(std::map<unsigned char, unsigned char*>& mosaicTextures,
	std::map<unsigned char, unsigned int>& mosaicTextureWidth,
	std::map<unsigned char, unsigned int>& mosaicTextureHeight)
{
	std::map<unsigned char, unsigned char*>::iterator iterTexture = mosaicTextures.begin();
	int bpp = 4;
	for (; iterTexture != mosaicTextures.end(); iterTexture++)
	{
		unsigned char lod = iterTexture->first;

		unsigned char* sourceImage = iterTexture->second;
		int sourceWidth = mosaicTextureWidth[iterTexture->first];
		int sourceHeight = mosaicTextureHeight[iterTexture->first];

		// resize width to type of 2^n
		int resizedWidth, resizedHeight;
		if ((sourceWidth & (sourceWidth - 1)) == 0)
			resizedWidth = sourceWidth;
		else
		{
			unsigned int prevPower = 1;
			while (sourceWidth >> prevPower != 1)
				prevPower++;

			unsigned int nextPower = prevPower + 1;
			resizedWidth = ((sourceWidth - (1 << prevPower)) > ((1 << nextPower) - sourceWidth)) ? 1 << nextPower : 1 << prevPower;
		}
		// resize height to type of 2^n
		if ((sourceHeight & (sourceHeight - 1)) == 0)
			resizedHeight = sourceHeight;
		else
		{
			unsigned int prevPower = 1;
			while (sourceHeight >> prevPower != 1)
				prevPower++;

			unsigned int nextPower = prevPower + 1;
			resizedHeight = ((sourceHeight - (1 << prevPower)) > ((1 << nextPower) - sourceHeight)) ? 1 << nextPower : 1 << prevPower;
		}

		int maxPixelLength = 1 << (MaxLod - lod + 7);
		if (resizedWidth > maxPixelLength)
			resizedWidth = maxPixelLength;
		if (resizedHeight > maxPixelLength)
			resizedHeight = maxPixelLength;

		int resizedImageSize = resizedWidth * resizedHeight * bpp;
		unsigned char* resizedImage = new unsigned char[resizedImageSize];
		memset(resizedImage, 0x00, resizedImageSize);
		stbir_resize_uint8(sourceImage, sourceWidth, sourceHeight, 0, resizedImage, resizedWidth, resizedHeight, 0, bpp);

		delete iterTexture->second;
		mosaicTextures[iterTexture->first] = resizedImage;
		mosaicTextureWidth[iterTexture->first] = resizedWidth;
		mosaicTextureHeight[iterTexture->first] = resizedHeight;
	}
}

void ConversionProcessor::changeXYPlaneCoordinateToRelativeCoordinateToBoundingBoxFootprintCenter(std::vector<gaia3d::TrianglePolyhedron*>& meshes,
	gaia3d::BoundingBox& bbox)
{
	double cx, cy, cz;
	bbox.getCenterPoint(cx, cy, cz);

	size_t meshCount = meshes.size();
	for (size_t i = 0; i < meshCount; i++)
	{
		gaia3d::TrianglePolyhedron* mesh = meshes[i];
		size_t vertexCount = mesh->getVertices().size();
		for (size_t j = 0; j < vertexCount; j++)
		{
			gaia3d::Vertex* vertex = mesh->getVertices()[j];

			vertex->position.x = vertex->position.x - cx;
			vertex->position.y = vertex->position.y - cy;
		}
	}

	bbox.minX = bbox.minX - cx;
	bbox.minY = bbox.minY - cy;
	bbox.maxX = bbox.maxX - cx;
	bbox.maxY = bbox.maxY - cy;
}

void ConversionProcessor::dropTrianglesOfSmallSizedEdge(std::vector<gaia3d::TrianglePolyhedron*>& meshes, double edgeMinSize)
{
	double squaredMinSize = edgeMinSize * edgeMinSize;

	double squaredEdgeLength0, squaredEdgeLength1, squaredEdgeLength2;
	double xDiff, yDiff, zDiff;
	size_t meshCount = meshes.size();
	for (size_t i = 0; i < meshCount; i++)
	{
		gaia3d::TrianglePolyhedron* mesh = meshes[i];
		size_t surfaceCount = mesh->getSurfaces().size();
		std::vector<gaia3d::Surface*> survivedSurfaces;
		std::vector<gaia3d::Vertex*> survivedVertices;
		std::map<gaia3d::Vertex*, size_t> survivedVertexMap;
		for (size_t j = 0; j < surfaceCount; j++)
		{
			gaia3d::Surface* surface = mesh->getSurfaces()[j];
			size_t triangleCount = surface->getTriangles().size();
			std::vector<gaia3d::Triangle*> survivedTriangles;
			for (size_t k = 0; k < triangleCount; k++)
			{
				gaia3d::Triangle* triangle = surface->getTriangles()[k];

				xDiff = (triangle->getVertices()[1]->position.x - triangle->getVertices()[0]->position.x);
				yDiff = (triangle->getVertices()[1]->position.y - triangle->getVertices()[0]->position.y);
				zDiff = (triangle->getVertices()[1]->position.z - triangle->getVertices()[0]->position.z);
				squaredEdgeLength0 = xDiff * xDiff + yDiff * yDiff + zDiff * zDiff;

				xDiff = (triangle->getVertices()[2]->position.x - triangle->getVertices()[1]->position.x);
				yDiff = (triangle->getVertices()[2]->position.y - triangle->getVertices()[1]->position.y);
				zDiff = (triangle->getVertices()[2]->position.z - triangle->getVertices()[1]->position.z);
				squaredEdgeLength1 = xDiff * xDiff + yDiff * yDiff + zDiff * zDiff;

				xDiff = (triangle->getVertices()[0]->position.x - triangle->getVertices()[2]->position.x);
				yDiff = (triangle->getVertices()[0]->position.y - triangle->getVertices()[2]->position.y);
				zDiff = (triangle->getVertices()[0]->position.z - triangle->getVertices()[2]->position.z);
				squaredEdgeLength2 = xDiff * xDiff + yDiff * yDiff + zDiff * zDiff;

				if (squaredEdgeLength0 < squaredMinSize || squaredEdgeLength1 < squaredMinSize || squaredEdgeLength2 < squaredMinSize)
				{
					delete triangle;
				}
				else
				{
					survivedTriangles.push_back(triangle);

					if (survivedVertexMap.find(triangle->getVertices()[0]) == survivedVertexMap.end())
					{
						survivedVertexMap[triangle->getVertices()[0]] = survivedVertices.size();
						survivedVertices.push_back(triangle->getVertices()[0]);
					}

					if (survivedVertexMap.find(triangle->getVertices()[1]) == survivedVertexMap.end())
					{
						survivedVertexMap[triangle->getVertices()[1]] = survivedVertices.size();
						survivedVertices.push_back(triangle->getVertices()[1]);
					}

					if (survivedVertexMap.find(triangle->getVertices()[2]) == survivedVertexMap.end())
					{
						survivedVertexMap[triangle->getVertices()[2]] = survivedVertices.size();
						survivedVertices.push_back(triangle->getVertices()[2]);
					}

					triangle->setVertexIndices(survivedVertexMap[triangle->getVertices()[0]],
						survivedVertexMap[triangle->getVertices()[1]],
						survivedVertexMap[triangle->getVertices()[2]]);
				}
			}

			surface->getTriangles().clear();

			if (!survivedTriangles.empty())
			{
				surface->getTriangles().assign(survivedTriangles.begin(), survivedTriangles.end());
				survivedSurfaces.push_back(surface);
			}
			else
				delete surface;
		}

		mesh->getSurfaces().clear();

		if (!survivedSurfaces.empty())
		{
			mesh->getSurfaces().assign(survivedSurfaces.begin(), survivedSurfaces.end());

			size_t vertexCount = mesh->getVertices().size();
			for (size_t j = 0; j < vertexCount; j++)
			{
				gaia3d::Vertex* vertex = mesh->getVertices()[j];
				if (survivedVertexMap.find(vertex) == survivedVertexMap.end())
					delete vertex;
			}

			mesh->getVertices().clear();
			mesh->getVertices().assign(survivedVertices.begin(), survivedVertices.end());
		}
	}
}

void ConversionProcessor::removeDuplicatedVerticesAndOverlappingTriangles(std::vector<gaia3d::TrianglePolyhedron*>& meshes,
	bool bCompareTexCoord,
	bool bCompareNormal)
{
	size_t meshCount = meshes.size();
	for (size_t i = 0; i < meshCount; i++)
	{
		gaia3d::TrianglePolyhedron* mesh = meshes[i];

		// collect vertex use case
		std::map<gaia3d::Vertex*, std::vector<gaia3d::Triangle*>> vertexUseCase;
		size_t surfaceCount = mesh->getSurfaces().size();
		for (size_t j = 0; j < surfaceCount; j++)
		{
			gaia3d::Surface* surface = mesh->getSurfaces()[j];
			size_t triangleCount = surface->getTriangles().size();
			for (size_t k = 0; k < triangleCount; k++)
			{
				gaia3d::Triangle* triangle = surface->getTriangles()[k];

				for (size_t m = 0; m < 3; m++)
				{
					gaia3d::Vertex* vertex = triangle->getVertices()[m];
					if (vertexUseCase.find(vertex) == vertexUseCase.end())
					{
						std::vector<gaia3d::Triangle*> trianglesUsingSameVertex;
						vertexUseCase[vertex] = trianglesUsingSameVertex;
					}

					vertexUseCase[vertex].push_back(triangle);
				}
			}
		}

		// spatial indexing on vertices for vertex search
		double maxBboxLength = mesh->getBoundingBox().getMaxLength();

		double leafOctreeSize = maxBboxLength / 10.0;
		if (leafOctreeSize < 3.0)
			leafOctreeSize = 3.0;

		gaia3d::PointDistributionOctree octree(NULL);
		octree.setSize(mesh->getBoundingBox().minX, mesh->getBoundingBox().minY, mesh->getBoundingBox().minZ,
			mesh->getBoundingBox().minX + maxBboxLength, mesh->getBoundingBox().minY + maxBboxLength, mesh->getBoundingBox().minZ + maxBboxLength);
		octree.vertices.assign(mesh->getVertices().begin(), mesh->getVertices().end());
		octree.makeTreeOfUnfixedDepth(leafOctreeSize, true);

		// remove duplicated vertices and overlapped triangles
		double error = 10E-6;
		double posError = 0.0001;
		double texError = 0.001;
		double norError = 0.001;
		std::map<gaia3d::Triangle*, int> overlappedTriangles;
		std::map<gaia3d::Vertex*, int> verticesToBeDelete;
		std::map<gaia3d::Vertex*, int> verticesUsedInOverlappedTriangles;
		size_t vertexCount = mesh->getVertices().size();
		for (size_t j = 0; j < vertexCount; j++)
		{
			gaia3d::Vertex* vertex = mesh->getVertices()[j];
			if (verticesToBeDelete.find(vertex) != verticesToBeDelete.end())
				continue;

			// find coincident vertices
			std::vector<gaia3d::Vertex*> coincidentVertices;
			gaia3d::PointDistributionOctree* leafOctree = octree.getIntersectedLeafOctree(vertex);
			if (leafOctree == NULL)
				continue;

			size_t octreeVertexCount = leafOctree->vertices.size();
			for (size_t k = 0; k < octreeVertexCount; k++)
			{
				gaia3d::Vertex* octreeVertex = leafOctree->vertices[k];
				if (vertex == octreeVertex)
					continue;

				// position checking
				if (octreeVertex->position.x < vertex->position.x - posError || octreeVertex->position.x > vertex->position.x + posError ||
					octreeVertex->position.y < vertex->position.y - posError || octreeVertex->position.y > vertex->position.y + posError ||
					octreeVertex->position.z < vertex->position.z - posError || octreeVertex->position.z > vertex->position.z + posError)
					continue;

				// texture coordinate checking
				if (bCompareTexCoord &&
					octreeVertex->textureCoordinate[0] < vertex->textureCoordinate[0] - texError ||
					octreeVertex->textureCoordinate[0] > vertex->textureCoordinate[0] + texError ||
					octreeVertex->textureCoordinate[1] < vertex->textureCoordinate[1] - texError ||
					octreeVertex->textureCoordinate[1] > vertex->textureCoordinate[1] + texError)
					continue;

				// normal checking
				if (bCompareNormal &&
					octreeVertex->normal.x < vertex->normal.x - norError || octreeVertex->normal.x > vertex->normal.x + norError ||
					octreeVertex->normal.y < vertex->normal.y - norError || octreeVertex->normal.y > vertex->normal.y + norError ||
					octreeVertex->normal.z < vertex->normal.z - norError || octreeVertex->normal.z > vertex->normal.z + norError)
					continue;

				coincidentVertices.push_back(octreeVertex);
			}

			// replace coincident vertices with original vertex
			// and update vertex use case
			size_t coincidentVertexCount = coincidentVertices.size();
			for (size_t k = 0; k < coincidentVertexCount; k++)
			{
				gaia3d::Vertex* coincidentVertex = coincidentVertices[k];
				if (vertexUseCase.find(coincidentVertex) == vertexUseCase.end())
					continue;

				size_t triangleCountOfCoincidentVertex = vertexUseCase[coincidentVertex].size();
				for (size_t m = 0; m < triangleCountOfCoincidentVertex; m++)
				{
					gaia3d::Triangle* triangleOfCoincidentVertex = vertexUseCase[coincidentVertex][m];
					if (triangleOfCoincidentVertex->getVertices()[0] == coincidentVertex)
					{
						if (triangleOfCoincidentVertex->getVertices()[1] != vertex &&
							triangleOfCoincidentVertex->getVertices()[2] != vertex)
						{
							triangleOfCoincidentVertex->getVertices()[0] = vertex;
							vertexUseCase[vertex].push_back(triangleOfCoincidentVertex);
							verticesToBeDelete[coincidentVertex] = 0;
						}
					}
					else if (triangleOfCoincidentVertex->getVertices()[1] == coincidentVertex)
					{
						if (triangleOfCoincidentVertex->getVertices()[2] != vertex &&
							triangleOfCoincidentVertex->getVertices()[0] != vertex)
						{
							triangleOfCoincidentVertex->getVertices()[1] = vertex;
							vertexUseCase[vertex].push_back(triangleOfCoincidentVertex);
							verticesToBeDelete[coincidentVertex] = 0;
						}
					}
					else
					{
						if (triangleOfCoincidentVertex->getVertices()[0] != vertex &&
							triangleOfCoincidentVertex->getVertices()[1] != vertex)
						{
							triangleOfCoincidentVertex->getVertices()[2] = vertex;
							vertexUseCase[vertex].push_back(triangleOfCoincidentVertex);
							verticesToBeDelete[coincidentVertex] = 0;
						}
					}
				}
			}

			// find overlapped triangles
			size_t vertexSharingTriangleCount = vertexUseCase[vertex].size();
			for (size_t k = 0; k < vertexSharingTriangleCount; k++)
			{
				gaia3d::Triangle* triangleA = vertexUseCase[vertex][k];
				if (overlappedTriangles.find(triangleA) != overlappedTriangles.end())
					continue;

				for (size_t m = k + 1; m < vertexSharingTriangleCount; m++)
				{
					gaia3d::Triangle* triangleB = vertexUseCase[vertex][m];
					if (triangleA == triangleB)
						continue;

					if (overlappedTriangles.find(triangleB) != overlappedTriangles.end())
						continue;

					for (char n = 0; n < 3; n++)
					{
						if ((triangleA->getVertices()[0] == triangleB->getVertices()[n % 3] &&
							triangleA->getVertices()[1] == triangleB->getVertices()[(n + 1) % 3]) ||
							(triangleA->getVertices()[1] == triangleB->getVertices()[(n + 1) % 3] &&
								triangleA->getVertices()[2] == triangleB->getVertices()[(n + 2) % 3]) ||
							(triangleA->getVertices()[2] == triangleB->getVertices()[(n + 2) % 3] &&
								triangleA->getVertices()[0] == triangleB->getVertices()[n % 3]))
						{
							overlappedTriangles[triangleB] = 0;
							verticesUsedInOverlappedTriangles[triangleB->getVertices()[0]] = 0;
							verticesUsedInOverlappedTriangles[triangleB->getVertices()[1]] = 0;
							verticesUsedInOverlappedTriangles[triangleB->getVertices()[2]] = 0;
							break;
						}
					}
				}
			}
		}

		// delete overlapping trianlges of this mesh and rebuild vertex id
		std::vector<gaia3d::Vertex*> survivedVertices;
		std::map<gaia3d::Vertex*, size_t> vertexId;
		for (size_t j = 0; j < surfaceCount; j++)
		{
			gaia3d::Surface* surface = mesh->getSurfaces()[j];
			size_t triangleCount = surface->getTriangles().size();
			std::vector<gaia3d::Triangle*> survivedTriangles;
			for (size_t k = 0; k < triangleCount; k++)
			{
				gaia3d::Triangle* triangle = surface->getTriangles()[k];
				if (overlappedTriangles.find(triangle) == overlappedTriangles.end())
				{
					for (int m = 0; m < 3; m++)
					{
						if (vertexId.find(triangle->getVertices()[m]) == vertexId.end())
						{
							vertexId[triangle->getVertices()[m]] = survivedVertices.size();
							survivedVertices.push_back(triangle->getVertices()[m]);
						}
					}
					triangle->setVertexIndices(vertexId[triangle->getVertices()[0]], vertexId[triangle->getVertices()[1]], vertexId[triangle->getVertices()[2]]);

					survivedTriangles.push_back(triangle);
				}
				else
				{
					delete triangle;
				}
			}

			surface->getTriangles().clear();
			surface->getTriangles().assign(survivedTriangles.begin(), survivedTriangles.end());
		}

		// delete coincident vertices of this mesh
		for (size_t j = 0; j < vertexCount; j++)
		{
			gaia3d::Vertex* vertex = mesh->getVertices()[j];
			if (vertexId.find(vertex) == vertexId.end())
				delete vertex;
		}
		mesh->getVertices().clear();
		mesh->getVertices().assign(survivedVertices.begin(), survivedVertices.end());
	}
}

void ConversionProcessor::makeLodTextureUsingOriginalTextureDirectly(unsigned char* originalTexture, int originalWidth, int originalHeight,
	std::map<unsigned char, unsigned char>& lodMadeOfOriginalMesh,
	std::map<unsigned char, unsigned char*>& netSurfaceTextures,
	std::map<unsigned char, unsigned int>& netSurfaceTextureWidth,
	std::map<unsigned char, unsigned int>& netSurfaceTextureHeight)
{
	int bpp = 4;
	std::map<unsigned char, unsigned char>::iterator iterLod = lodMadeOfOriginalMesh.begin();
	for (; iterLod != lodMadeOfOriginalMesh.end(); iterLod++)
	{
		unsigned char lod = iterLod->first;

		int resizedWidth, resizedHeight;
		switch (lod)
		{
		case 2:
		{
			resizedWidth = originalWidth < 1024 ? originalWidth : 1024;
			resizedHeight = originalHeight < 1024 ? originalHeight : 1024;
		}
		break;
		case 3:
		{
			resizedWidth = originalWidth < 512 ? originalWidth : 512;
			resizedHeight = originalHeight < 512 ? originalHeight : 512;
		}
		break;
		case 4:
		{
			resizedWidth = originalWidth < 256 ? originalWidth : 256;
			resizedHeight = originalHeight < 256 ? originalHeight : 256;
		}
		break;
		case 5:
		{
			resizedWidth = originalWidth < 128 ? originalWidth : 128;
			resizedHeight = originalHeight < 128 ? originalHeight : 128;
		}
		break;
		}

		int resizedImageSize = resizedWidth * resizedHeight * bpp;
		unsigned char* resizedImage = new unsigned char[resizedImageSize];
		memset(resizedImage, 0x00, resizedImageSize);
		stbir_resize_uint8(originalTexture, originalWidth, originalHeight, 0, resizedImage, resizedWidth, resizedHeight, 0, bpp);

		netSurfaceTextures[lod] = resizedImage;
		netSurfaceTextureWidth[lod] = resizedWidth;
		netSurfaceTextureHeight[lod] = resizedHeight;
	}
}

void ConversionProcessor::divideOriginalTextureIntoSmallerSize(unsigned char* originalTexture, int originalWidth, int originalHeight,
	gaia3d::SpatialOctreeBox& octree,
	std::map<std::string, unsigned char*>& results,
	std::map<std::string, unsigned int>& resultWidths,
	std::map<std::string, unsigned int>& resultHeights,
	std::map<std::string, std::string>& resultTextureInfo)
{
	resultTextureInfo.clear();

	gaia3d::Image2DUtils* utils = new gaia3d::Image2DUtils;

	gaia3d::Image2D image_original;
	image_original.Set_Image(originalTexture, originalWidth, originalHeight);
	image_original.m_dimensions = 4; // RGBA.!!!

	std::vector<gaia3d::OctreeBox*> leafBoxes;
	octree.getAllLeafBoxes(leafBoxes, true);

	size_t leafOctreeCount = leafBoxes.size();
	for (size_t i = 0; i < leafOctreeCount; i++)
	{
		gaia3d::SpatialOctreeBox* leafOctree = (gaia3d::SpatialOctreeBox*)leafBoxes[i];
		utils->MULTISPLITTIMAGE_Delete_Image2DSplitDatas(); // init

		// make new texture file name
		std::string textureFileName = "ImageOctree_" + std::to_string(leafOctree->octreeId) + ".jpg";
		resultTextureInfo[textureFileName] = textureFileName;

		size_t meshCount = leafOctree->meshes.size();
		for (size_t j = 0; j < meshCount; j++)
		{
			gaia3d::TrianglePolyhedron* mesh = leafOctree->meshes[j];

			// replace old texture info of newer one
			mesh->getStringAttributes()[std::string(TextureName)] = textureFileName;

			size_t surfaceCount = mesh->getSurfaces().size();
			for (size_t k = 0; k < surfaceCount; k++)
			{
				gaia3d::Surface* surface = mesh->getSurfaces()[k];
				size_t triangleCount = surface->getTriangles().size();
				for (size_t m = 0; m < triangleCount; m++)
				{
					gaia3d::Triangle* triangle = surface->getTriangles()[m];
					gaia3d::Rectangle rectangleImage;

					// calculate the texture-coords rectangle of the triangle.***
					rectangleImage.Set_Init(triangle->getVertices()[0]->textureCoordinate[0], triangle->getVertices()[0]->textureCoordinate[1]);
					rectangleImage.Add_Point(triangle->getVertices()[1]->textureCoordinate[0], triangle->getVertices()[1]->textureCoordinate[1]);
					rectangleImage.Add_Point(triangle->getVertices()[2]->textureCoordinate[0], triangle->getVertices()[2]->textureCoordinate[1]);

					utils->MULTISPLITTIMAGE_Add_ImageRectangle(&rectangleImage, triangle);
				}
			}

			utils->MULTISPLITTIMAGE_Recombine_ImageRectangles();
		}

		utils->MULTISPLITTIMAGE_Recombine_ImageRectangles();

		// now, if there are multiples rectanglesImages, then make a oneMosaic.***
		utils->MULTISPLITTIMAGE_Make_SplittedMosaic(); // splitMosaic is smaller than the original.***

													   // now, for this lowestOctree, make the splitted image texCoords.***
													   // To do this, we need the new textureImage boundingRectangle.***
		gaia3d::Rectangle newImageBoundingRect;
		size_t imageRectanglesCount = utils->m_vec_image2DSplitDatas.size();
		for (size_t j = 0; j < imageRectanglesCount; j++)
		{
			gaia3d::Image2DSplitData* image2dSplitData = utils->m_vec_image2DSplitDatas[j];
			gaia3d::Rectangle* splittedRect = image2dSplitData->m_rectangleImage_splitted;
			if (j == 0)
			{
				newImageBoundingRect.CopyFrom(splittedRect);
			}
			else
			{
				newImageBoundingRect.Add_Rectangle(splittedRect);
			}
		}

		double newImage_width = newImageBoundingRect.Get_Width();
		double newImage_height = newImageBoundingRect.Get_Height();
		// note: the original width = 1.0, but the newImage is smaller than the original.***

		// now, recalculate the texCoords.***
		for (size_t j = 0; j < imageRectanglesCount; j++)
		{
			gaia3d::Image2DSplitData* image2dSplitData = utils->m_vec_image2DSplitDatas[j];
			gaia3d::Rectangle* splitDataRect = image2dSplitData->m_rectangleImage_splitted;
			gaia3d::Rectangle* originalDataRect = image2dSplitData->m_rectangleImage_original;

			double splitDataRect_width = splitDataRect->Get_Width();
			double splitDataRect_height = splitDataRect->Get_Height();

			// Note: splitDataRect_size = originalDataRect_size.***
			double originalDataRect_width = originalDataRect->Get_Width();
			double originalDataRect_height = originalDataRect->Get_Height();

			std::vector<gaia3d::Vertex*> vec_vertices;
			image2dSplitData->Get_Vertices(vec_vertices);
			size_t vertexCount = vec_vertices.size();
			for (size_t k = 0; k < vertexCount; k++)
			{
				gaia3d::Vertex* vertex = vec_vertices[k];
				double* texCoord_0 = vertex->textureCoordinate;

				double u, v;
				double u2, v2;

				// point 0.***
				// 1rst, recalculate referencing only by "image2dSplitData-rectangle", NO by newTexture-rectangle.***
				u = (texCoord_0[0] - originalDataRect->m_minX) / originalDataRect_width;
				v = (texCoord_0[1] - originalDataRect->m_minY) / originalDataRect_height;

				// now calculate referencing by newTexture-rectangle.***
				u2 = (splitDataRect->m_minX + u * originalDataRect_width) / newImage_width;
				v2 = (splitDataRect->m_minY + v * originalDataRect_height) / newImage_height;
				texCoord_0[0] = u2;
				texCoord_0[1] = v2;
			}
		}

		// make a sub-texture of this octree

		gaia3d::Rectangle smallImageRect;
		// 1rst, calculate the splittedImageRectangle.***
		for (size_t j = 0; j < imageRectanglesCount; j++)
		{
			gaia3d::Image2DSplitData* splitData = utils->m_vec_image2DSplitDatas[j];
			if (splitData->m_vec_triangles.size() > 0)
			{
				if (j == 0)
				{
					smallImageRect.CopyFrom(splitData->m_rectangleImage_splitted);
				}
				else
				{
					smallImageRect.Add_Rectangle(splitData->m_rectangleImage_splitted);
				}
			}
		}

		// now, create the smallImage.***
		double smallImageRectWidthDouble = smallImageRect.Get_Width();
		if (smallImageRectWidthDouble < 0.0)
			smallImageRectWidthDouble = 0.0;
		double smallImageRectHeightDouble = smallImageRect.Get_Height();
		if (smallImageRectHeightDouble < 0.0)
			smallImageRectHeightDouble = 0.0;

		int smallImageWidth = int(smallImageRectWidthDouble * double(originalWidth));
		int smallImageHeight = int(smallImageRectHeightDouble * double(originalHeight));
		unsigned char* smallImageArray = new unsigned char[smallImageWidth * smallImageHeight * 4]; // RGBA.!!!
		memset(smallImageArray, 90, smallImageWidth * smallImageHeight * 4 * sizeof(unsigned char)); // RGBA.!!!
		gaia3d::Image2D smallImage;
		smallImage.Set_Image(smallImageArray, smallImageWidth, smallImageHeight);

		// Now, take the original part of the image and insert into smallImage.***
		for (size_t j = 0; j < imageRectanglesCount; j++)
		{
			gaia3d::Image2DSplitData* splitData = utils->m_vec_image2DSplitDatas[j];

			if (splitData->m_vec_triangles.size() > 0)
			{
				// 1) Take the splittedImage from original image.***
				// The image_original is in RGB, but we take a region in RGBA.***
				gaia3d::Rectangle* splittedOriginalRect = splitData->m_rectangleImage_original;
				gaia3d::Rectangle* splittedNewPosRect = splitData->m_rectangleImage_splitted;
				gaia3d::Rectangle splittedNewPosRectNormalized;
				splittedNewPosRectNormalized.CopyFrom(splittedNewPosRect);
				splittedNewPosRectNormalized.m_minX /= smallImageRectWidthDouble;
				splittedNewPosRectNormalized.m_maxX /= smallImageRectWidthDouble;
				splittedNewPosRectNormalized.m_minY /= smallImageRectHeightDouble;
				splittedNewPosRectNormalized.m_maxY /= smallImageRectHeightDouble;

				gaia3d::Image2D splittedImage_RGBA;
				utils->Get_Region(&image_original, splittedOriginalRect, &splittedImage_RGBA);

				// 2) insert the splittedImage into smallImage.***
				utils->InsertImage_RGBA(&smallImage, &splittedNewPosRectNormalized, &splittedImage_RGBA);
			}
		}

		// now resize the result so that width/height is type of 2^n
		unsigned char* tempTexture = smallImage.m_image;
		int tempWidth = smallImage.m_imageWidth;
		int tempHeight = smallImage.m_imageHeight;
		int widthResized;
		int heightResized;

		// resize width into a shape of power of 2
		if ((tempWidth & (tempWidth - 1)) == 0)
			widthResized = tempWidth;
		else
		{
			unsigned int prevPower = 1;
			while (tempWidth >> prevPower != 1)
				prevPower++;

			unsigned int nextPower = prevPower + 1;
			widthResized = ((tempWidth - (1 << prevPower)) > ((1 << nextPower) - tempWidth)) ? 1 << nextPower : 1 << prevPower;
		}

		// resize height into a shape of power of 2
		if ((tempHeight & (tempHeight - 1)) == 0)
			heightResized = tempHeight;
		else
		{
			unsigned int prevPower = 1;
			while (tempHeight >> prevPower != 1)
				prevPower++;

			unsigned int nextPower = prevPower + 1;
			heightResized = ((tempHeight - (1 << prevPower)) > ((1 << nextPower) - tempHeight)) ? 1 << nextPower : 1 << prevPower;
		}

		int bpp = 4;
		int resizedImageSize = widthResized * heightResized * bpp;
		unsigned char* resizedImage = new unsigned char[resizedImageSize];
		memset(resizedImage, 0x00, resizedImageSize);
		stbir_resize_uint8(tempTexture, tempWidth, tempHeight, 0, resizedImage, widthResized, heightResized, 0, bpp);

		results[textureFileName] = resizedImage;
		resultWidths[textureFileName] = widthResized;
		resultHeights[textureFileName] = heightResized;



		utils->MULTISPLITTIMAGE_Delete_Image2DSplitDatas();
	}

	delete utils;
}

void ConversionProcessor::reuseOriginalMeshForRougherLods(gaia3d::SpatialOctreeBox& octree)
{
	// std::map<unsigned char, gaia3d::TrianglePolyhedron*> netSurfaceMeshes;
	std::vector<gaia3d::OctreeBox*> leafBoxes;
	octree.getAllLeafBoxes(leafBoxes, true);

	// merge mehses to make lod2 data
	size_t leafBoxCount = leafBoxes.size();
	for (size_t i = 0; i < leafBoxCount; i++)
	{
		gaia3d::SpatialOctreeBox* leafBox = (gaia3d::SpatialOctreeBox*)leafBoxes[i];

		gaia3d::TrianglePolyhedron* fakeNsm = new gaia3d::TrianglePolyhedron;
		gaia3d::Surface* newSurface = new gaia3d::Surface;
		fakeNsm->getSurfaces().push_back(newSurface);
		fakeNsm->setHasTextureCoordinates(true);
		fakeNsm->setHasNormals(true);

		leafBox->netSurfaceMesh = fakeNsm;

		size_t meshCount = leafBox->meshes.size();
		for (size_t j = 0; j < meshCount; j++)
		{
			gaia3d::TrianglePolyhedron* mesh = leafBox->meshes[j];
			size_t offset = fakeNsm->getVertices().size();

			// copy vertices
			size_t vertexCount = mesh->getVertices().size();
			for (size_t k = 0; k < vertexCount; k++)
			{
				gaia3d::Vertex* sourceVertex = mesh->getVertices()[k];
				gaia3d::Vertex* targetVertex = new gaia3d::Vertex;
				targetVertex->position = sourceVertex->position;
				targetVertex->normal = sourceVertex->normal;
				targetVertex->color = sourceVertex->color;
				targetVertex->textureCoordinate[0] = sourceVertex->textureCoordinate[0];
				targetVertex->textureCoordinate[1] = sourceVertex->textureCoordinate[1];
				fakeNsm->getVertices().push_back(targetVertex);
			}

			// copy triangles
			size_t surfaceCount = mesh->getSurfaces().size();
			for (size_t k = 0; k < surfaceCount; k++)
			{
				gaia3d::Surface* surface = mesh->getSurfaces()[k];
				size_t triangleCount = surface->getTriangles().size();
				for (size_t m = 0; m < triangleCount; m++)
				{
					gaia3d::Triangle* sourceTriangle = surface->getTriangles()[m];
					gaia3d::Triangle* targetTriangle = new gaia3d::Triangle;
					targetTriangle->setNormal(sourceTriangle->getNormal()->x, sourceTriangle->getNormal()->y, sourceTriangle->getNormal()->z);
					targetTriangle->setVertexIndices(sourceTriangle->getVertexIndices()[0] + offset,
						sourceTriangle->getVertexIndices()[1] + offset,
						sourceTriangle->getVertexIndices()[2] + offset);
					targetTriangle->setVertices(fakeNsm->getVertices()[targetTriangle->getVertexIndices()[0]],
						fakeNsm->getVertices()[targetTriangle->getVertexIndices()[1]],
						fakeNsm->getVertices()[targetTriangle->getVertexIndices()[2]]);
					newSurface->getTriangles().push_back(targetTriangle);
				}
			}
		}
	}

	// merge nsm of each spatial octree to make lod 3 to maxLod data
	for (unsigned char i = 3; i <= MaxLod; i++)
	{
		netSurfaceMeshes[i] = new gaia3d::TrianglePolyhedron;
		netSurfaceMeshes[i]->getSurfaces().push_back(new gaia3d::Surface);
		netSurfaceMeshes[i]->setHasTextureCoordinates(true);
		netSurfaceMeshes[i]->setHasNormals(true);
	}

	for (size_t i = 0; i < leafBoxCount; i++)
	{
		gaia3d::SpatialOctreeBox* leafBox = (gaia3d::SpatialOctreeBox*)leafBoxes[i];
		gaia3d::TrianglePolyhedron* sourceMesh = leafBox->netSurfaceMesh;

		size_t offset = netSurfaceMeshes.begin()->second->getVertices().size();

		// copy vertices
		size_t vertexCount = sourceMesh->getVertices().size();
		for (size_t j = 0; j < vertexCount; j++)
		{
			gaia3d::Vertex* sourceVertex = sourceMesh->getVertices()[j];

			for (unsigned char k = 3; k <= MaxLod; k++)
			{
				gaia3d::Vertex* targetVertex = new gaia3d::Vertex;
				targetVertex->position = sourceVertex->position;
				targetVertex->normal = sourceVertex->normal;
				targetVertex->color = sourceVertex->color;
				targetVertex->textureCoordinate[0] = sourceVertex->textureCoordinate[0];
				targetVertex->textureCoordinate[1] = sourceVertex->textureCoordinate[1];
				netSurfaceMeshes[k]->getVertices().push_back(targetVertex);
			}
		}

		// copy triangles
		size_t surfaceCount = sourceMesh->getSurfaces().size();
		for (size_t j = 0; j < surfaceCount; j++)
		{
			gaia3d::Surface* surface = sourceMesh->getSurfaces()[j];
			size_t triangleCount = surface->getTriangles().size();
			for (size_t k = 0; k < triangleCount; k++)
			{
				gaia3d::Triangle* sourceTriangle = surface->getTriangles()[k];

				for (unsigned char m = 3; m <= MaxLod; m++)
				{
					gaia3d::Triangle* targetTriangle = new gaia3d::Triangle;
					targetTriangle->setNormal(sourceTriangle->getNormal()->x, sourceTriangle->getNormal()->y, sourceTriangle->getNormal()->z);
					targetTriangle->setVertexIndices(sourceTriangle->getVertexIndices()[0] + offset,
						sourceTriangle->getVertexIndices()[1] + offset,
						sourceTriangle->getVertexIndices()[2] + offset);
					targetTriangle->setVertices(netSurfaceMeshes[m]->getVertices()[targetTriangle->getVertexIndices()[0]],
						netSurfaceMeshes[m]->getVertices()[targetTriangle->getVertexIndices()[1]],
						netSurfaceMeshes[m]->getVertices()[targetTriangle->getVertexIndices()[2]]);
					netSurfaceMeshes[m]->getSurfaces().front()->getTriangles().push_back(targetTriangle);
				}
			}
		}
	}

	// make fake textures for lod 2~MaxLod
	//std::map<unsigned char, unsigned char*> netSurfaceTextures;
	//std::map<unsigned char, int> netSurfaceTextureWidth;
	//std::map<unsigned char, int> netSurfaceTextureHeight;
	for (unsigned char i = 2; i <= MaxLod; i++)
	{
		netSurfaceTextures[i] = new unsigned char[4];
		(netSurfaceTextures[i])[0] = (netSurfaceTextures[i])[1] = (netSurfaceTextures[i])[2] = 127;
		(netSurfaceTextures[i])[3] = 255;
		netSurfaceTextureWidth[i] = 1;
		netSurfaceTextureHeight[i] = 1;
	}

}

void ConversionProcessor::makeSixFaceTexturesOnBox(
	std::vector<gaia3d::TrianglePolyhedron*>& meshes,
	gaia3d::BoundingBox& bbox,
	std::vector<unsigned char*>& output,
	unsigned int textureWidth, unsigned int textureHeight,
	unsigned int shaderProgram,
	std::map<std::string, unsigned int>& bindingResult)
{
	double backupXRot = scv->m_xRot, backupYRot = scv->m_yRot, backupZRot = scv->m_zRot;
	double backupXPos = scv->m_xPos, backupYPos = scv->m_yPos, backupZPos = scv->m_zPos;
	float backupNRange = scv->m_nRange;

	for (unsigned int i = 0; i < 6; i++)
	{
		unsigned char* imageBuffer = NULL;
		imageBuffer = makeFaceTextureOnBox(i, meshes, bbox, textureWidth, textureHeight, shaderProgram, bindingResult);
		output.push_back(imageBuffer);
	}

	scv->m_xRot = backupXRot; scv->m_yRot = backupYRot; scv->m_zRot = backupZRot;
	scv->m_xPos = backupXPos; scv->m_yPos = backupYPos; scv->m_zPos = backupZPos;
	scv->m_nRange = backupNRange;
}
unsigned char* ConversionProcessor::makeFaceTextureOnBox(
	unsigned int faceType,
	std::vector<gaia3d::TrianglePolyhedron*>& meshes,
	gaia3d::BoundingBox& bbox,
	unsigned int textureWidth, unsigned int textureHeight,
	unsigned int shaderProgram,
	std::map<std::string, unsigned int>& bindingResult)
{
	// setup viewport
	scv->tp_projection = PROJECTION_ORTHO;
	glViewport(0, 0, textureWidth, textureHeight);

	gaia3d::Point3D centerPoint; bbox.getCenterPoint(centerPoint.x, centerPoint.y, centerPoint.z);
	float drawingBufferWidth = (float)textureWidth, drawingBufferHeight = (float)textureHeight;

	unsigned int wa = textureWidth, ha = textureHeight;

	// Mini define_space_of_visualitzation.***********************************************************************************************
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// FaceType (0 = Top), (1 = Bottom), (2 = Front), (3 = Rear), (4 = Left), (5 = Right).***
	float frustumNear = 0.0f;
	float frustumFar = 0.0f;
	float frustumTop = 0.0f, frustumBottom = 0.0f, frustumLeft = 0.0f, frustumRight = 0.0f;
	switch (faceType)
	{
	case 0:
	case 1:
	{
		frustumNear = (float)(-(bbox.getZLength() / 2.0));
		frustumFar = (float)(bbox.getZLength() / 2.0);

		frustumBottom = (float)(-(bbox.getYLength() / 2.0));
		frustumTop = (float)(bbox.getYLength() / 2.0);

		frustumLeft = (float)(-(bbox.getXLength() / 2.0));
		frustumRight = (float)(bbox.getXLength() / 2.0);
	}
	break;
	case 2:
	case 3:
	{
		frustumNear = (float)(-(bbox.getYLength() / 2.0));
		frustumFar = (float)(bbox.getYLength() / 2.0);

		frustumBottom = (float)(-(bbox.getZLength() / 2.0));
		frustumTop = (float)(bbox.getZLength() / 2.0);

		frustumLeft = (float)(-(bbox.getXLength() / 2.0));
		frustumRight = (float)(bbox.getXLength() / 2.0);
	}
	break;
	case 4:
	case 5:
	{
		frustumNear = (float)(-(bbox.getXLength() / 2.0));
		frustumFar = (float)(bbox.getXLength() / 2.0);

		frustumBottom = (float)(-(bbox.getZLength() / 2.0));
		frustumTop = (float)(bbox.getZLength() / 2.0);

		frustumLeft = (float)(-(bbox.getYLength() / 2.0));
		frustumRight = (float)(bbox.getYLength() / 2.0);
	}
	break;
	}

	//wglMakeCurrent(scv->m_myhDC, scv->m_hRC);

	glViewport(0, 0, (GLsizei)textureWidth, (GLsizei)textureHeight); // Set the viewport 
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glOrtho(-scv->m_nRange*wa / ha, scv->m_nRange*wa / ha, -scv->m_nRange, scv->m_nRange, frustumNear, frustumFar); // Note that wa/ha = 1.***
	glOrtho(frustumLeft * wa / ha, frustumRight * wa / ha, frustumBottom, frustumTop, frustumNear, frustumFar); // Note that wa/ha = 1.***
	glMatrixMode(GL_MODELVIEW);
	// End Mini define_space_of_visualitzation.--------------------------------------------------------------------------------------------

	glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
	// Clear the screen and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Reset the model matrix
	//glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// setup target position
	double xRot = 0.0, yRot = 0.0, zRot = 0.0;
	switch (faceType)
	{
	case 0:
	{
		xRot = 0.0; yRot = 0.0; zRot = 0.0;
	}
	break;
	case 1:
	{
		xRot = 180.0; yRot = 0.0; zRot = 0.0;
	}
	break;
	case 2:
	{
		xRot = -90.0; yRot = 0.0; zRot = 0.0;
	}
	break;
	case 3:
	{
		xRot = 90.0; yRot = 0.0; zRot = 0.0;
	}
	break;
	case 4:
	{
		xRot = 0.0; yRot = 90.0; zRot = 0.0;
	}
	break;
	case 5:
	{
		xRot = 0.0; yRot = -90.0; zRot = 0.0;
	}
	break;
	}
	scv->m_xRot = xRot; scv->m_yRot = yRot; scv->m_zRot = zRot;
	scv->m_xPos = -centerPoint.x;
	scv->m_yPos = -centerPoint.y;
	scv->m_zPos = -centerPoint.z;

	glReadBuffer(GL_BACK);
	glDrawBuffer(GL_BACK);
	glDisable(GL_LIGHTING);

	glRotatef((float)scv->m_xRot, 1.0f, 0.0f, 0.0f);
	glRotatef((float)scv->m_yRot, 0.0f, 1.0f, 0.0f);
	glRotatef((float)scv->m_zRot, 0.0f, 0.0f, 1.0f);

	glTranslatef((float)scv->m_xPos, (float)scv->m_yPos, (float)scv->m_zPos);

	// at this point, make netSurfacTexture
	unsigned char* texture = new unsigned char[(unsigned int)(textureWidth * textureHeight * 4)];
	memset(texture, 0x00, (unsigned int)(textureWidth * textureHeight * 4));
	drawMeshesWithTextures(meshes, bindingResult, shaderProgram);
	//drawTrianglesForTexture(meshes, bindingResult);
	glReadPixels(0, 0,textureWidth, textureHeight, GL_RGBA, GL_UNSIGNED_BYTE, texture);

	glEnable(GL_LIGHTING);
	glPopMatrix();

	return texture;
}

void ConversionProcessor::mergeFaceTexsturesIntoSingleOne(
	std::map<gaia3d::SpatialOctreeBox*, std::vector<unsigned char*>>& faceTextures,
	unsigned int faceImageWidth, unsigned int faceImageHeight,
	unsigned char*& mosaicTexture,
	unsigned int& mosaicWidth, unsigned int& mosaicHeight)
{
	size_t boxCount = faceTextures.size();

	size_t totalFaceTextureCount = boxCount * 6;

	size_t mosaicCol, mosaicRow;
	mosaicCol = mosaicRow = ((size_t)sqrt(totalFaceTextureCount)) + 1;

	size_t expandedPixel = (size_t)(faceImageWidth * 0.025);
	mosaicWidth = (unsigned int)((faceImageWidth + expandedPixel * 2) * mosaicCol);
	mosaicHeight = (unsigned int)((faceImageHeight + expandedPixel * 2) * mosaicRow);
	size_t mosaicTextureSize = mosaicWidth * mosaicHeight * 4;
	mosaicTexture = new unsigned char[mosaicTextureSize];
	memset(mosaicTexture, 0x00, sizeof(unsigned char) * mosaicTextureSize);

	std::map<gaia3d::SpatialOctreeBox*, std::vector<unsigned char*>>::iterator itr = faceTextures.begin();
	unsigned int currentCol = 0, currentRow = 0;
	for (; itr != faceTextures.end(); itr++)
	{
		gaia3d::SpatialOctreeBox* octree = itr->first;
		std::vector<unsigned char*>& subTextures = itr->second;

		if (octree->prettySkinMesh == NULL)
			continue;

		float textureCoordinate[4];
		for (size_t i = 0; i < subTextures.size(); i++)
		{
			memset(textureCoordinate, 0x00, sizeof(float) * 4);
			insertSubTextureIntoMosaicTexture(
				mosaicTexture,
				mosaicCol, mosaicRow,
				currentCol, currentRow,
				subTextures[i],
				faceImageWidth, faceImageHeight, expandedPixel,
				textureCoordinate);

			calculateTextureCoordinates(octree->prettySkinMesh, (unsigned int)i,
				textureCoordinate[0], textureCoordinate[2], textureCoordinate[1], textureCoordinate[3],
				float(octree->minX), float(octree->maxX),
				float(octree->minY), float(octree->maxY),
				float(octree->minZ), float(octree->maxZ));

			// after inserting data, increment currentCol.***
			currentCol++;

			if (currentCol >= mosaicCol)
			{
				currentCol = 0;
				currentRow++;
			}
		}

		octree->prettySkinMesh->setHasTextureCoordinates(true);
		octree->prettySkinMesh->setHasNormals(true);
		
		flipTextureCoordinateY(octree->prettySkinMesh);
	}
}

void ConversionProcessor::insertSubTextureIntoMosaicTexture(
	unsigned char*& mosaicTexture,
	unsigned int mosaicNumCols,
	unsigned int mosaicNumRows,
	unsigned int insertCol,
	unsigned int insertRow,
	unsigned char* subTexture,
	unsigned int subTextureWidth,
	unsigned int subTextureHeight,
	unsigned int pixelMargin,
	float textureCoordinate[])
{
	unsigned char* dataRGBA = subTexture;
	unsigned int dataRGBA_width = subTextureWidth;
	unsigned int dataRGBA_height = subTextureHeight;

	unsigned int mosaicTexturePixelWidth = mosaicNumCols * (dataRGBA_width + pixelMargin*2);
	unsigned int mosaicTexturePixelHeight = mosaicNumRows * (dataRGBA_height + pixelMargin*2);

	// first, find the leftDownCornerPixelPosition of the dataRGBA into the mosaicTexture.***
	unsigned int dataRGBA_leftDownPixelCol = insertCol * (dataRGBA_width + pixelMargin * 2);
	unsigned int dataRGBA_leftDownPixelRow = insertRow * (dataRGBA_height + pixelMargin * 2);

	unsigned int currentDataCol = 0;
	unsigned int currentDataRow = 0;
	unsigned int dataRGBA_size = dataRGBA_width * dataRGBA_height;
	for (unsigned int i = 0; i < dataRGBA_size; i++)
	{
		//unsigned char* sourcePixel = dataRGBA + i * 4;
		unsigned char r = dataRGBA[i * 4];
		unsigned char g = dataRGBA[i * 4 + 1];
		unsigned char b = dataRGBA[i * 4 + 2];
		unsigned char a = dataRGBA[i * 4 + 3];

		// must find the pixel position in mosaicTexture.***
		unsigned int insertPixelCol = dataRGBA_leftDownPixelCol + currentDataCol + pixelMargin;
		unsigned int insertPixelRow = dataRGBA_leftDownPixelRow + currentDataRow + pixelMargin;
		unsigned int mosaicTexPixelIdx = insertPixelCol + insertPixelRow * mosaicTexturePixelWidth;

		// change the mosaicTexture's pixel value.***
		//unsigned char* targetPixel = (*mosaicTexture) + mosaicTexPixelIdx * 4;
		//memcpy(targetPixel, sourcePixel, sizeof(unsigned char) * 4);
		mosaicTexture[mosaicTexPixelIdx * 4] = r;
		mosaicTexture[mosaicTexPixelIdx * 4 + 1] = g;
		mosaicTexture[mosaicTexPixelIdx * 4 + 2] = b;
		mosaicTexture[mosaicTexPixelIdx * 4 + 3] = a;

		currentDataCol++;
		if (currentDataCol == dataRGBA_width)
		{
			currentDataCol = 0;
			currentDataRow++;
		}
	}

	// fill border pixels of this sub texture frame(each border edge thickness : pixelMargin)
	for (unsigned int i = 0; i < pixelMargin; i++)
	{
		unsigned char r1, r2;
		unsigned char g1, g2;
		unsigned char b1, b2;
		unsigned char a1, a2;

		// top&bottom border
		for (unsigned int j = 0; j < dataRGBA_width+pixelMargin*2; j++)
		{
			if (j < pixelMargin)
			{
				// original left bottom corner pixel
				r1 = dataRGBA[0];
				g1 = dataRGBA[1];
				b1 = dataRGBA[2];
				a1 = dataRGBA[3];

				// original left top corner pixel
				r2 = dataRGBA[dataRGBA_width * (dataRGBA_height - 1) * 4];
				g2 = dataRGBA[dataRGBA_width * (dataRGBA_height - 1) * 4 + 1];
				b2 = dataRGBA[dataRGBA_width * (dataRGBA_height - 1) * 4 + 2];
				a2 = dataRGBA[dataRGBA_width * (dataRGBA_height - 1) * 4 + 3];
			}
			else if(j >= dataRGBA_width + pixelMargin)
			{
				// original right bottom corner pixel
				r1 = dataRGBA[(dataRGBA_width - 1) * 4];
				g1 = dataRGBA[(dataRGBA_width - 1) * 4 + 1];
				b1 = dataRGBA[(dataRGBA_width - 1) * 4 + 2];
				a1 = dataRGBA[(dataRGBA_width - 1) * 4 + 3];

				// original right top corner pixel
				r2 = dataRGBA[(dataRGBA_size - 1) * 4];
				g2 = dataRGBA[(dataRGBA_size - 1) * 4 + 1];
				b2 = dataRGBA[(dataRGBA_size - 1) * 4 + 2];
				a2 = dataRGBA[(dataRGBA_size - 1) * 4 + 3];
			}
			else
			{
				// original bottom edge pixel
				r1 = dataRGBA[(j - pixelMargin) * 4];
				g1 = dataRGBA[(j - pixelMargin) * 4 + 1];
				b1 = dataRGBA[(j - pixelMargin) * 4 + 2];
				a1 = dataRGBA[(j - pixelMargin) * 4 + 3];

				// original top edge pixel
				r2 = dataRGBA[(j - pixelMargin + dataRGBA_width * (dataRGBA_height - 1)) * 4];
				g2 = dataRGBA[(j - pixelMargin + dataRGBA_width * (dataRGBA_height - 1)) * 4 + 1];
				b2 = dataRGBA[(j - pixelMargin + dataRGBA_width * (dataRGBA_height - 1)) * 4 + 2];
				a2 = dataRGBA[(j - pixelMargin + dataRGBA_width * (dataRGBA_height - 1)) * 4 + 3];
			}

			// fill border pixels of mosaic texture
			// bottom
			unsigned int targetBottomEdgeIndex = (j + dataRGBA_leftDownPixelCol) + mosaicTexturePixelWidth * (i + dataRGBA_leftDownPixelRow);
			mosaicTexture[targetBottomEdgeIndex * 4] = r1;
			mosaicTexture[targetBottomEdgeIndex * 4 + 1] = g1;
			mosaicTexture[targetBottomEdgeIndex * 4 + 2] = b1;
			mosaicTexture[targetBottomEdgeIndex * 4 + 3] = a1;

			// top
			unsigned int targetTopEdgeIndex = (j + dataRGBA_leftDownPixelCol) + mosaicTexturePixelWidth * (i+dataRGBA_height+pixelMargin + dataRGBA_leftDownPixelRow);
			mosaicTexture[targetTopEdgeIndex * 4] = r2;
			mosaicTexture[targetTopEdgeIndex * 4 + 1] = g2;
			mosaicTexture[targetTopEdgeIndex * 4 + 2] = b2;
			mosaicTexture[targetTopEdgeIndex * 4 + 3] = a2;
		}

		// left&right border
		for (unsigned int j = pixelMargin; j < dataRGBA_width + pixelMargin; j++)
		{
			// original left edge pixel
			r1 = dataRGBA[dataRGBA_width * (j - pixelMargin) * 4];
			g1 = dataRGBA[dataRGBA_width * (j - pixelMargin) * 4 + 1];
			b1 = dataRGBA[dataRGBA_width * (j - pixelMargin) * 4 + 2];
			a1 = dataRGBA[dataRGBA_width * (j - pixelMargin) * 4 + 3];

			// original right edge pixel
			r2 = dataRGBA[(dataRGBA_width-1 + dataRGBA_width * (j - pixelMargin)) * 4];
			g2 = dataRGBA[(dataRGBA_width - 1 + dataRGBA_width * (j - pixelMargin)) * 4 + 1];
			b2 = dataRGBA[(dataRGBA_width - 1 + dataRGBA_width * (j - pixelMargin)) * 4 + 2];
			a2 = dataRGBA[(dataRGBA_width - 1 + dataRGBA_width * (j - pixelMargin)) * 4 + 3];

			// fill border pixels of mosaic texture
			// left
			unsigned int targetLeftEdgeIndex = (i+ dataRGBA_leftDownPixelCol) + mosaicTexturePixelWidth * (j + dataRGBA_leftDownPixelRow);
			mosaicTexture[targetLeftEdgeIndex * 4] = r1;
			mosaicTexture[targetLeftEdgeIndex * 4 + 1] = g1;
			mosaicTexture[targetLeftEdgeIndex * 4 + 2] = b1;
			mosaicTexture[targetLeftEdgeIndex * 4 + 3] = a1;
			// right
			unsigned int targetRightEdgeIndex = (i+ dataRGBA_width+pixelMargin + dataRGBA_leftDownPixelCol) + mosaicTexturePixelWidth * (j + dataRGBA_leftDownPixelRow);
			mosaicTexture[targetRightEdgeIndex * 4] = r2;
			mosaicTexture[targetRightEdgeIndex * 4 + 1] = g2;
			mosaicTexture[targetRightEdgeIndex * 4 + 2] = b2;
			mosaicTexture[targetRightEdgeIndex * 4 + 3] = a2;
		}
	}

	// calculate texture coordinates of net surface meshes for mosaic texture
	int textoreCoord_offSet = 0;

	float minS = (float)(dataRGBA_leftDownPixelCol + textoreCoord_offSet) / (float)mosaicTexturePixelWidth;
	float maxS = (float)(dataRGBA_leftDownPixelCol + dataRGBA_width - textoreCoord_offSet) / (float)mosaicTexturePixelWidth;
	float minT = (float)(dataRGBA_leftDownPixelRow + textoreCoord_offSet) / (float)mosaicTexturePixelHeight;
	float maxT = (float)(dataRGBA_leftDownPixelRow + dataRGBA_height - textoreCoord_offSet) / (float)mosaicTexturePixelHeight;

	float textMarginS = (maxS - minS) * 0.0464f;
	float textMarginT = (maxT - minT) * 0.0464f;

	textureCoordinate[0] = minS + textMarginS;
	textureCoordinate[1] = minT + textMarginT;
	textureCoordinate[2] = maxS - textMarginS;
	textureCoordinate[3] = maxT - textMarginT;
}

void ConversionProcessor::calculateTextureCoordinates(gaia3d::TrianglePolyhedron* polyhedron,
	unsigned int cubeface,
	float minS, float maxS, float minT, float maxT,
	float minX, float maxX, float minY, float maxY, float minZ, float maxZ)
{
	float mosaic_sRange = maxS - minS;
	float mosaic_tRange = maxT - minT;

	size_t surfaceCount = polyhedron->getSurfaces().size();
	for (size_t h = 0; h < surfaceCount; h++)
	{
		gaia3d::Surface* surface = polyhedron->getSurfaces()[h];
		size_t triangleCount = surface->getTriangles().size();

		for (size_t i = 0; i < triangleCount; i++)
		{
			gaia3d::Triangle* tri = surface->getTriangles()[i];

			// which component of normal vector has the max magnitude
			unsigned int triangleNormalMainComponent = 0;
			gaia3d::Point3D* normal = tri->getNormal();
			double nx = fabs(normal->x);
			double ny = fabs(normal->y);
			double nz = fabs(normal->z);
			unsigned char mainComponent = (nx > ny) ? ((nx > nz) ? 0 : 2) : ((ny > nz) ? 1 : 2);
			switch (mainComponent)
			{
			case 0: // x component
			{
				if (normal->x > 0.0) // toward right
					triangleNormalMainComponent = 5;
				else // toward left
					triangleNormalMainComponent = 4;
			}
			break;
			case 1: // y component
			{
				if (normal->y > 0.0) // toward rear
					triangleNormalMainComponent = 3;
				else // toward front
					triangleNormalMainComponent = 2;
			}
			break;
			default : // z component
			{
				if (normal->z > 0.0) // toward top
					triangleNormalMainComponent = 0;
				else // toward bottom
					triangleNormalMainComponent = 1;
			}
			}

			if (triangleNormalMainComponent != cubeface)
				continue;

			if (cubeface == 0) // top
			{
				float xRange = maxX - minX;
				float yRange = maxY - minY;

				for (int j = 0; j < 3; j++)
				{
					gaia3d::Vertex* vertex = tri->getVertices()[j];

					gaia3d::Point3D* p = &(vertex->position);
					double x = p->x;
					double y = p->y;
					double s = (x - minX) / xRange;
					double t = (y - minY) / yRange;

					// now correct s, t to put inside the mosaic.***
					double mosaicS = minS + s * mosaic_sRange;
					double mosaicT = minT + t * mosaic_tRange;

					vertex->textureCoordinate[0] = mosaicS; vertex->textureCoordinate[1] = mosaicT;
				}
			}
			else if (cubeface == 1) // bottom
			{
				float xRange = maxX - minX;
				float yRange = maxY - minY;

				for (int j = 0; j < 3; j++)
				{
					gaia3d::Vertex* vertex = tri->getVertices()[j];

					gaia3d::Point3D* p = &(vertex->position);
					double x = p->x;
					double y = p->y;
					double s = (x - minX) / xRange;
					double t = (y - minY) / yRange;
					t = 1.0 - t;

					// now correct s, t to put inside the mosaic.***
					double mosaicS = minS + s * mosaic_sRange;
					double mosaicT = minT + t * mosaic_tRange;
					vertex->textureCoordinate[0] = mosaicS; vertex->textureCoordinate[1] = mosaicT;
				}
			}
			else if (cubeface == 2) // front
			{
				float xRange = maxX - minX;
				float zRange = maxZ - minZ;

				for (int j = 0; j < 3; j++)
				{
					gaia3d::Vertex* vertex = tri->getVertices()[j];

					gaia3d::Point3D* p = &(vertex->position);
					double x = p->x;
					double z = p->z;
					double s = (x - minX) / xRange;
					double t = (z - minZ) / zRange;

					// now correct s, t to put inside the mosaic.***
					double mosaicS = minS + s * mosaic_sRange;
					double mosaicT = minT + t * mosaic_tRange;

					vertex->textureCoordinate[0] = mosaicS; vertex->textureCoordinate[1] = mosaicT;
				}
			}
			else if (cubeface == 3) // rear
			{
				float xRange = maxX - minX;
				float zRange = maxZ - minZ;

				for (int j = 0; j < 3; j++)
				{
					gaia3d::Vertex* vertex = tri->getVertices()[j];

					gaia3d::Point3D* p = &(vertex->position);
					double x = p->x;
					double z = p->z;
					double s = (x - minX) / xRange;
					double t = (z - minZ) / zRange;
					t = 1.0 - t;

					// now correct s, t to put inside the mosaic.***
					double mosaicS = minS + s * mosaic_sRange;
					double mosaicT = minT + t * mosaic_tRange;

					vertex->textureCoordinate[0] = mosaicS; vertex->textureCoordinate[1] = mosaicT;
				}
			}
			else if (cubeface == 4) // left
			{
				float yRange = maxY - minY;
				float zRange = maxZ - minZ;

				for (int j = 0; j < 3; j++)
				{
					gaia3d::Vertex* vertex = tri->getVertices()[j];

					gaia3d::Point3D* p = &(vertex->position);
					double y = p->y;
					double z = p->z;
					double t = (y - minY) / yRange;
					double s = (z - minZ) / zRange;

					//s = 1.0 - s;
					// now correct s, t to put inside the mosaic.***
					double mosaicS = minS + s * mosaic_sRange;
					double mosaicT = minT + t * mosaic_tRange;

					vertex->textureCoordinate[0] = mosaicS; vertex->textureCoordinate[1] = mosaicT;
				}
			}
			else if (cubeface == 5) // right
			{
				float yRange = maxY - minY;
				float zRange = maxZ - minZ;

				for (int j = 0; j < 3; j++)
				{
					gaia3d::Vertex* vertex = tri->getVertices()[j];

					gaia3d::Point3D* p = &(vertex->position);
					double y = p->y;
					double z = p->z;
					double t = (y - minY) / yRange;
					double s = (z - minZ) / zRange;
					s = 1.0 - s;

					// now correct s, t to put inside the mosaic.***
					double mosaicS = minS + s * mosaic_sRange;
					double mosaicT = minT + t * mosaic_tRange;

					vertex->textureCoordinate[0] = mosaicS; vertex->textureCoordinate[1] = mosaicT;
				}
			}
		}
	}
}

void ConversionProcessor::flipTextureCoordinateY(gaia3d::TrianglePolyhedron* polyhedron)
{
	size_t vertexCount = polyhedron->getVertices().size();
	for (size_t i = 0; i < vertexCount; i++)
	{
		gaia3d::Vertex* vertex = polyhedron->getVertices()[i];
		vertex->textureCoordinate[1] = 1.0 - vertex->textureCoordinate[1];
	}
}

