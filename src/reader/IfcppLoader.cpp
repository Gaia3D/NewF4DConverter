/**
* Implementation of the IfcppLoader class
*/
#include "IfcppLoader.h"

#include <ifcpp/model/StatusCallback.h>

class MessageWrapper
{
public:
	static void slotMessageWrapper(void* obj_ptr, shared_ptr<StatusCallback::Message> m)
	{
		if (m)
		{
			if (m->m_message_type != StatusCallback::MESSAGE_TYPE_PROGRESS_VALUE && m->m_message_type != StatusCallback::MESSAGE_TYPE_PROGRESS_TEXT)
			{
				std::wcout << m->m_message_text << std::endl;
			}
		}
	}
};

IfcppLoader::IfcppLoader()
{
	bVertexReduction = false;
	bAttributesExtraction = false;

	Json::Value projectProperty(Json::objectValue);
	projectPropertyRoot["project"] = projectProperty;

	Json::Value objectPropertyList(Json::arrayValue);
	objectPropertyRoot["objects"] = objectPropertyList;
}

IfcppLoader::~IfcppLoader()
{
	size_t polyhedronCount = polyhedrons.size();
	size_t surfaceCount;
	for (size_t i = 0; i < polyhedronCount; i++)
	{
		delete[] polyhedrons[i]->vertices;
		surfaceCount = polyhedrons[i]->surfaces.size();
		for (size_t j = 0; j < surfaceCount; j++)
		{
			delete[] polyhedrons[i]->surfaces[j]->triangleIndices;
			delete polyhedrons[i]->surfaces[j];
		}

		polyhedrons[i]->surfaces.clear();

		delete polyhedrons[i];
	}

	polyhedrons.clear();
}

bool IfcppLoader::loadIfcFile(std::wstring& filePath)
{
	// initializing
	shared_ptr<MessageWrapper> mw(new MessageWrapper());
	shared_ptr<BuildingModel> ifc_model(new BuildingModel());
	shared_ptr<GeometryConverter> geometry_converter(new GeometryConverter(ifc_model));
	shared_ptr<ReaderSTEP> reader(new ReaderSTEP());

	reader->setMessageCallBack(mw.get(), &MessageWrapper::slotMessageWrapper);
	geometry_converter->setMessageCallBack(mw.get(), &MessageWrapper::slotMessageWrapper);

	// loading
	reader->loadModelFromFile(filePath, ifc_model);
	geometry_converter->convertGeometry();

	// conversion raw data into geometries of OSG type
	//osg::ref_ptr<osg::Switch> model_switch = new osg::Switch();
	//geometry_converter->createGeometryOSG(model_switch);

	// convert Carve geometry to Opengl format
	std::cout << "Converting geometry to OpenGL format ..." << std::endl;

	double volume_all_products = 0;
	shared_ptr<ProductShapeData> ifc_project_data;

	std::map<int, shared_ptr<ProductShapeData> >& map_shape_data = geometry_converter->getShapeInputData();
	std::map<int, shared_ptr<ProductShapeData> >::iterator it;
	for (it = map_shape_data.begin(); it != map_shape_data.end(); ++it)
	{
		// STEP entity id:
		int entity_id = it->first;

		// shape data
		shared_ptr<ProductShapeData>& shape_data = it->second;

		weak_ptr<IfcObjectDefinition>& ifc_object_def_weak = shape_data->m_ifc_object_definition;
		if (ifc_object_def_weak.expired())
		{
			continue;
		}
		shared_ptr<IfcObjectDefinition> ifc_object_def(ifc_object_def_weak);
		if (dynamic_pointer_cast<IfcFeatureElementSubtraction>(ifc_object_def))
		{
			// geometry will be created in method subtractOpenings
			continue;
		}
		else if (dynamic_pointer_cast<IfcProject>(ifc_object_def))
		{
			ifc_project_data = shape_data;
		}

		// IfcProduct(abstract type)
		//shared_ptr<IfcProduct> ifc_product(shape_data->m_ifc_product);
		shared_ptr<IfcProduct> ifc_product = dynamic_pointer_cast<IfcProduct>(ifc_object_def);
		if (!ifc_product)
		{
			continue;
		}

		if (!ifc_product->m_Representation)
		{
			continue;
		}
		
		// filtering out IfcProduct of IfcSpace type
		shared_ptr<IfcSpace> Ifc_Space = dynamic_pointer_cast<IfcSpace>(ifc_product);
		if (Ifc_Space)
		{
			continue;
		}
		/*
		// filtering out IfcProduct of IfcSite type
		shared_ptr<IfcSite> Ifc_Site = dynamic_pointer_cast<IfcSite>(ifc_product);
		if (!Ifc_Site)
		{
			continue;
		}
		*/
		int product_id = ifc_product->m_entity_id;

		std::cout << "#" << product_id << "=" << ifc_product->className() << " group" << std::endl;

		carve::math::Matrix mat = shape_data->getTransform();
		// for each IfcProduct, there can be mulitple geometric representation items:
		std::vector<shared_ptr<RepresentationData> >& vec_representations = shape_data->m_vec_representations;
		for (size_t i_representation = 0; i_representation < vec_representations.size(); ++i_representation)
		{
			shared_ptr<RepresentationData>& representation_data = vec_representations[i_representation];
			if (representation_data->m_ifc_representation.expired())
			{
				continue;
			}
			shared_ptr<IfcRepresentation> ifc_representation(representation_data->m_ifc_representation);
			
			int representation_id = ifc_representation->m_entity_id;

			// a representation item can have multiple item shapes
			std::vector<shared_ptr<ItemShapeData> >& vec_item_data = representation_data->m_vec_item_data;
			for (size_t i_item = 0; i_item < vec_item_data.size(); ++i_item)
			{
				shared_ptr<ItemShapeData>& item_data = vec_item_data[i_item];

				// appearance data for getting color
				std::vector<shared_ptr<AppearanceData> > vec_items_appearances = item_data->m_vec_item_appearances;

				std::vector<shared_ptr<carve::mesh::MeshSet<3> > > allMeshsets;
				allMeshsets.insert(allMeshsets.end(), item_data->m_meshsets.begin(), item_data->m_meshsets.end());
				allMeshsets.insert(allMeshsets.end(), item_data->m_meshsets_open.begin(), item_data->m_meshsets_open.end());
				Polyhedron* polyhedron;
				Surface* surface;
				for (size_t i_meshset = 0; i_meshset < allMeshsets.size(); ++i_meshset)
				{
					// A meshset == A Polyhedron
					shared_ptr<carve::mesh::MeshSet<3> >& meshset = allMeshsets[i_meshset];
					CSG_Adapter::retriangulateMeshSet(meshset);
					polyhedron = NULL;

					////////////////////////////////////
					if (bVertexReduction)
					{
						// vertices of this meshset(to add all vertices to a Polyhedron)
						std::map<carve::mesh::Vertex<3> *, size_t> vertices;
						std::map<carve::mesh::Vertex<3> *, size_t>::iterator vertexIter;

						// A meshset is composed of multiple meshes.( mesh = surface)
						std::vector<carve::mesh::Mesh<3>* >& vec_meshes = meshset->meshes;
						for (size_t i_mesh = 0; i_mesh < vec_meshes.size(); ++i_mesh)
						{
							// Mesh == Surface
							carve::mesh::Mesh<3>* mesh = vec_meshes[i_mesh];
							surface = NULL;

							// vertex indices to be used to compose of triangles
							std::vector<size_t> indices;

							// A mesh is composed of multiple faces. (face == triangle)
							std::vector<carve::mesh::Face<3>* >& vec_faces = mesh->faces;
							for (size_t i_face = 0; i_face < vec_faces.size(); ++i_face)
							{
								// Face == Triangle
								carve::mesh::Face<3>* face = vec_faces[i_face];

								// iterate through edges:
								carve::mesh::Edge<3>* edge = face->edge;
								size_t vertexIndex;
								do
								{
									// start vertices of each edge
									// 각 edge의 start vertext만 instance화 한다.
									// 왜냐하면 한 edge의 end vertex는 다음 edge의 start vertex이므로.
									carve::mesh::Vertex<3>* vertex_begin = edge->v1();

									vertexIter = vertices.find(vertex_begin);
									if (vertexIter == vertices.end())
									{
										vertexIndex = vertices.size();
										vertices.insert(std::map<carve::mesh::Vertex<3>*, size_t>::value_type(vertex_begin, vertexIndex));
									}
									else
										vertexIndex = vertexIter->second;

									indices.push_back(vertexIndex);

									// 각 edge의 start vertext만 instance화 한다.
									// 왜냐하면 한 edge의 end vertex는 다음 edge의 start vertex이므로.

									edge = edge->next;
								} while (edge != face->edge);
							}

							// fill triangle count and triangle vertex indices
							if (indices.size() / 3 == 0)
								continue;

							surface = new Surface;

							surface->triangleCount = indices.size() / 3;
							surface->triangleIndices = new size_t[3 * surface->triangleCount];
							memset(surface->triangleIndices, 0x00, sizeof(size_t) * 3 * surface->triangleCount);
							for (size_t iIndex = 0; iIndex < 3 * surface->triangleCount; iIndex++)
								surface->triangleIndices[iIndex] = indices[iIndex];

							// add this surface to this polyhedron
							if (polyhedron == NULL)
								polyhedron = new Polyhedron;

							polyhedron->surfaces.push_back(surface);
						}

						if (polyhedron == NULL)
							continue;

						// fill vertex count and vertex position information
						std::map<size_t, carve::mesh::Vertex<3> *> verticesSorted;
						for (vertexIter = vertices.begin(); vertexIter != vertices.end(); vertexIter++)
							verticesSorted.insert(std::map<size_t, carve::mesh::Vertex<3> *>::value_type(vertexIter->second, vertexIter->first));

						polyhedron->vertexCount = verticesSorted.size();
						polyhedron->vertices = new double[3 * polyhedron->vertexCount];
						memset(polyhedron->vertices, 0x00, sizeof(double) * 3 * polyhedron->vertexCount);
						for (size_t vIndex = 0; vIndex < polyhedron->vertexCount; vIndex++)
						{
							polyhedron->vertices[3 * vIndex] = verticesSorted[vIndex]->v.x;
							polyhedron->vertices[3 * vIndex + 1] = verticesSorted[vIndex]->v.y;
							polyhedron->vertices[3 * vIndex + 2] = verticesSorted[vIndex]->v.z;
						}
					}
					////////////////////////
					else
					{
						// vertices of this meshset(to add all vertices to a Polyhedron)
						std::vector<carve::mesh::Vertex<3> *> vertices;

						// A meshset is composed of multiple meshes.( mesh = surface)
						std::vector<carve::mesh::Mesh<3>* >& vec_meshes = meshset->meshes;
						for (size_t i_mesh = 0; i_mesh < vec_meshes.size(); ++i_mesh)
						{
							// Mesh == Surface
							carve::mesh::Mesh<3>* mesh = vec_meshes[i_mesh];
							surface = NULL;

							size_t prevVertexCount = vertices.size();

							// A mesh is composed of multiple faces. (face == triangle)
							std::vector<carve::mesh::Face<3>* >& vec_faces = mesh->faces;
							for (size_t i_face = 0; i_face < vec_faces.size(); ++i_face)
							{
								// Face == Triangle
								carve::mesh::Face<3>* face = vec_faces[i_face];

								// iterate through edges:
								carve::mesh::Edge<3>* edge = face->edge;
								do
								{
									// start vertices of each edge
									// 각 edge의 start vertext만 instance화 한다.
									// 왜냐하면 한 edge의 end vertex는 다음 edge의 start vertex이므로.
									carve::mesh::Vertex<3>* vertex_begin = edge->v1();

									vertices.push_back(vertex_begin);

									edge = edge->next;
								} while (edge != face->edge);
							}

							if ((vertices.size() - prevVertexCount) / 3 == 0)
								continue;

							surface = new Surface;

							// fill triangle count and triangle vertex indices
							surface->triangleCount = (vertices.size() - prevVertexCount) / 3;
							surface->triangleIndices = new size_t[3 * surface->triangleCount];
							memset(surface->triangleIndices, 0x00, sizeof(size_t) * 3 * surface->triangleCount);
							for (size_t iIndex = 0; iIndex < 3 * surface->triangleCount; iIndex++)
								surface->triangleIndices[iIndex] = prevVertexCount + iIndex;

							// add this surface to this polyhedron
							if (polyhedron == NULL)
								polyhedron = new Polyhedron;

							polyhedron->surfaces.push_back(surface);
						}

						if (polyhedron == NULL)
							continue;

						polyhedron->vertexCount = vertices.size();
						polyhedron->vertices = new double[3 * polyhedron->vertexCount];
						memset(polyhedron->vertices, 0x00, sizeof(double) * 3 * polyhedron->vertexCount);

						for (size_t vIndex = 0; vIndex < polyhedron->vertexCount; vIndex++)
						{
							carve::geom::vector<3> vertex = vertices[vIndex]->v;
							vertex *= mat;

							polyhedron->vertices[3 * vIndex] = vertex.x;
							polyhedron->vertices[3 * vIndex + 1] = vertex.y;
							polyhedron->vertices[3 * vIndex + 2] = vertex.z;
						}
					}

					// get representative color of a Polyhedron, if exist
					if (vec_items_appearances.size() > 0)
					{
						polyhedron->color[0] = vec_items_appearances[0]->m_color_diffuse.m_r;
						polyhedron->color[1] = vec_items_appearances[0]->m_color_diffuse.m_g;
						polyhedron->color[2] = vec_items_appearances[0]->m_color_diffuse.m_b;
						polyhedron->color[3] = vec_items_appearances[0]->m_color_diffuse.m_a;
					}
					else
						memset(polyhedron->color, 0x00, sizeof(float) * 4);

					// extract and allocate guid into this polyhedron
					polyhedron->guid = ifc_product->m_GlobalId->m_value;

					// add this polyhedron
					polyhedrons.push_back(polyhedron);
				}
			}
		}

		// TODO : 
		//if (this->bAttributesExtraction && checkIfPropertiesCanBeExtracted(ifc_product->m_entity_enum))
		//	loadObjectAttributes(ifc_product, objectPropertyRoot);
	}

	/*
	// in case there are IFC entities that are not in the spatial structure
	const std::map<int, shared_ptr<BuildingObject> >& objects_outside_spatial_structure = geometry_converter->getObjectsOutsideSpatialStructure();
	if (objects_outside_spatial_structure.size() > 0)
	{
	std::cout << "IfcProduct objects outside spatial structure" << std::endl;
	}
	*/

	return true;
}

bool IfcppLoader::loadOnlyPropertiesFromIfc(std::wstring& filePath)
{
	// initializing
	shared_ptr<MessageWrapper> mw(new MessageWrapper());
	shared_ptr<BuildingModel> ifc_model(new BuildingModel());
	shared_ptr<GeometryConverter> geometry_converter(new GeometryConverter(ifc_model));
	shared_ptr<ReaderSTEP> reader(new ReaderSTEP());

	reader->setMessageCallBack(mw.get(), &MessageWrapper::slotMessageWrapper);
	geometry_converter->setMessageCallBack(mw.get(), &MessageWrapper::slotMessageWrapper);

	// loading
	reader->loadModelFromFile(filePath, ifc_model);
	geometry_converter->convertGeometry();

	// conversion raw data into geometries of OSG type
	//osg::ref_ptr<osg::Switch> model_switch = new osg::Switch();
	//geometry_converter->createGeometryOSG(model_switch);

	// contains the VEF graph for each IfcProduct:
	std::map<int, shared_ptr<ProductShapeData> >& map_shape_data = geometry_converter->getShapeInputData();
	double volume_all_products = 0;

	std::map<int, shared_ptr<ProductShapeData> >::iterator it;
	for (it = map_shape_data.begin(); it != map_shape_data.end(); ++it)
	{
		// STEP entity id:
		int entity_id = it->first;

		// shape data
		shared_ptr<ProductShapeData>& shape_data = it->second;

		weak_ptr<IfcObjectDefinition>& ifc_object_def_weak = shape_data->m_ifc_object_definition;
		if (ifc_object_def_weak.expired())
		{
			continue;
		}
		shared_ptr<IfcObjectDefinition> ifc_object_def(ifc_object_def_weak);
		if (dynamic_pointer_cast<IfcFeatureElementSubtraction>(ifc_object_def))
		{
			// geometry will be created in method subtractOpenings
			continue;
		}

		// IfcProduct(abstract type)
		//shared_ptr<IfcProduct> ifc_product(shape_data->m_ifc_product);
		shared_ptr<IfcProduct> ifc_product = dynamic_pointer_cast<IfcProduct>(ifc_object_def);
		if (!ifc_product)
		{
			continue;
		}

		if (!ifc_product->m_Representation)
		{
			continue;
		}

		// filtering out IfcProduct of IfcSpace type
		shared_ptr<IfcSpace> Ifc_Space = dynamic_pointer_cast<IfcSpace>(ifc_product);
		if (!Ifc_Space)
		{
			continue;
		}

		// filtering out IfcProduct of IfcSite type
		shared_ptr<IfcSite> Ifc_Site = dynamic_pointer_cast<IfcSite>(ifc_product);
		if (!Ifc_Site)
		{
			continue;
		}

		// TODO :
		//if (!checkIfPropertiesCanBeExtracted(ifc_product->m_entity_enum))
		//	continue;

		//loadObjectAttributes(ifc_product, objectPropertyRoot);
	}

#ifdef TMPTEST
	std::map<std::string, std::string>::iterator iter = attributeMap.begin();
	for (; iter != attributeMap.end(); iter++)
		attributeTypes.push_back(iter->first);
#endif

	return true;
}

void IfcppLoader::setVertexReductionMode(bool bOn)
{
	bVertexReduction = bOn;
}

void IfcppLoader::setAttributesExtraction(bool bOn)
{
	bAttributesExtraction = bOn;
}

size_t IfcppLoader::getPolyhedronCount()
{
	return polyhedrons.size();
}

float* IfcppLoader::getRepresentativeColor(size_t polyhedronIndex)
{
	return polyhedrons[polyhedronIndex]->color;
}

std::wstring IfcppLoader::getGuid(size_t polyhedronIndex)
{
	return polyhedrons[polyhedronIndex]->guid;
}

size_t IfcppLoader::getVertexCount(size_t polyhedronIndex)
{
	return polyhedrons[polyhedronIndex]->vertexCount;
}

double* IfcppLoader::getVertexPositions(size_t polyhedronIndex)
{
	return polyhedrons[polyhedronIndex]->vertices;
}

size_t IfcppLoader::getSurfaceCount(size_t polyhedronIndex)
{
	return polyhedrons[polyhedronIndex]->surfaces.size();
}

size_t IfcppLoader::getTrialgleCount(size_t polyhedronIndex, size_t surfaceIndex)
{
	return polyhedrons[polyhedronIndex]->surfaces[surfaceIndex]->triangleCount;
}

size_t* IfcppLoader::getTriangleIndices(size_t polyhedronIndex, size_t surfaceIndex)
{
	return polyhedrons[polyhedronIndex]->surfaces[surfaceIndex]->triangleIndices;
}

std::string IfcppLoader::getObjectAttributes()
{
	std::string result;

	Json::StyledWriter writer;
	result = writer.write(objectPropertyRoot);

	return result;
}

std::string IfcppLoader::getProjectAttributes()
{
	std::string result;

	Json::StyledWriter writer;
	result = writer.write(projectPropertyRoot);

	return result;
}

void IfcppLoader::loadProjectAttributes()
{}
/*
void IfcppLoader::loadObjectAttributes(shared_ptr<IfcProduct> ifcProduct, Json::Value& root)
{
	Json::Value properties(Json::objectValue);

	// guid
	std::string guid = gaia3d::StringUtility::convertWideStringToUtf8(ifcProduct->m_GlobalId->m_value);
	properties["guid"] = guid;

	// element type
	IfcPPEntityEnum ppEnum = ifcProduct->m_entity_enum;
	std::string elemType1(ifcProduct->className());
	properties["entityType"] = elemType1;
	properties["propertySets"] = Json::Value(Json::arrayValue);

	// properties
	size_t propertySetCount = ifcProduct->m_IsDefinedBy_inverse.size();
	for (size_t i = 0; i < propertySetCount; i++)
	{
		shared_ptr<IfcRelDefinesByProperties> propertySetWrapper = dynamic_pointer_cast<IfcRelDefinesByProperties>(ifcProduct->m_IsDefinedBy_inverse[i].lock());

		shared_ptr<IfcPropertySet> propertySet = dynamic_pointer_cast<IfcPropertySet>(propertySetWrapper->m_RelatingPropertyDefinition);

		Json::Value aSet(Json::objectValue);

		if (!propertySet->m_Name->m_value.empty())
			aSet["propertySetName"] = gaia3d::StringUtility::convertWideStringToUtf8(propertySet->m_Name->m_value); // @@@property set name
		else
			aSet["propertySetName"] = std::string("unknown");

		Json::Value propertyAggr(Json::objectValue);
		size_t propertyCount = propertySet->m_HasProperties.size();
		for (size_t j = 0; j < propertyCount; j++)
		{

			// property key
			std::string keyName;
			if (!propertySet->m_HasProperties[j]->m_Name->m_value.empty())
				keyName = gaia3d::StringUtility::convertWideStringToUtf8(propertySet->m_HasProperties[j]->m_Name->m_value);
			else
				keyName = std::string("key") + std::to_string(j);

			// property value
			if (dynamic_pointer_cast<IfcSimpleProperty>(propertySet->m_HasProperties[j]) != NULL)
			{
				if (dynamic_pointer_cast<IfcPropertySingleValue>(propertySet->m_HasProperties[j]) != NULL)
				{
					shared_ptr<IfcPropertySingleValue> singleValue = dynamic_pointer_cast<IfcPropertySingleValue>(propertySet->m_HasProperties[j]);

					Json::Value valueObject(Json::objectValue);
					this->parsePropertySingleValue(valueObject, singleValue->m_NominalValue);

#ifdef TMPTEST
					attributeMap.insert(std::map<std::string, std::string>::value_type(std::string(singleValue->m_NominalValue->className()), std::string(singleValue->m_NominalValue->className())));
#endif
					if (singleValue->m_Unit != NULL)
					{
						if (dynamic_pointer_cast<IfcNamedUnit>(singleValue->m_Unit) != NULL)
						{
							shared_ptr<IfcNamedUnit> namedUnit = dynamic_pointer_cast<IfcNamedUnit>(singleValue->m_Unit);
							valueObject["unit"] = namedUnit->m_UnitType->m_enum;
						}
						else if (dynamic_pointer_cast<IfcMonetaryUnit>(singleValue->m_Unit) != NULL)
						{
							shared_ptr<IfcMonetaryUnit> monetaryUnit = dynamic_pointer_cast<IfcMonetaryUnit>(singleValue->m_Unit);
							if (!monetaryUnit->m_Currency->m_value.empty())
								valueObject["unit"] = gaia3d::StringUtility::convertWideStringToUtf8(monetaryUnit->m_Currency->m_value);
							else
								valueObject["unit"] = std::string("empty currency unit");
						}
						else if (dynamic_pointer_cast<IfcDerivedUnit>(singleValue->m_Unit) != NULL)
						{
							//shared_ptr<IfcDerivedUnit> derivedUnit = dynamic_pointer_cast<IfcDerivedUnit>(singleValue->m_Unit);
							valueObject["unit"] = std::string("derived unit");
						}
						else
							valueObject["unit"] = std::string("unknown unit");
					}
					propertyAggr[keyName] = valueObject;
				}
				else
					propertyAggr[keyName] = std::string("unknown : ") + std::string(propertySet->m_HasProperties[j]->className());
			}
			else
				propertyAggr[keyName] = std::string("unknown : ") + std::string(propertySet->m_HasProperties[j]->className());
		}
		aSet["properties"] = propertyAggr;

		properties["propertySets"].append(aSet);
	}

	root["objects"].append(properties);
}
*/
// TODO:
/*
bool IfcppLoader::checkIfPropertiesCanBeExtracted(IfcPPEntityEnum ppEnum)
{
	switch (ppEnum)
	{
	case IfcPPEntityEnum::IFCANNOTATION:
	case IfcPPEntityEnum::IFCANNOTATIONFILLAREA:
		return false;
	}
	return true;
}
*/

void IfcppLoader::parsePropertySingleValue(Json::Value& valueObject, shared_ptr<IfcValue> value)
{
	if (dynamic_pointer_cast<IfcAreaMeasure>(value) != NULL)
		valueObject["value"] = dynamic_pointer_cast<IfcAreaMeasure>(value)->m_value;
	else if (dynamic_pointer_cast<IfcBoolean>(value) != NULL)
		valueObject["value"] = dynamic_pointer_cast<IfcBoolean>(value)->m_value;
	else if (dynamic_pointer_cast<IfcIdentifier>(value) != NULL)
	{
		if (dynamic_pointer_cast<IfcIdentifier>(value)->m_value.empty())
			valueObject["value"] = std::string("");
		else
			valueObject["value"] = gaia3d::StringUtility::convertWideStringToUtf8(dynamic_pointer_cast<IfcIdentifier>(value)->m_value);
	}
	else if (dynamic_pointer_cast<IfcInteger>(value) != NULL)
		valueObject["value"] = dynamic_pointer_cast<IfcInteger>(value)->m_value;
	else if (dynamic_pointer_cast<IfcLabel>(value) != NULL)
	{
		if (dynamic_pointer_cast<IfcLabel>(value)->m_value.empty())
			valueObject["value"] = std::string("");
		else
			valueObject["value"] = gaia3d::StringUtility::convertWideStringToUtf8(dynamic_pointer_cast<IfcLabel>(value)->m_value);
	}
	else if (dynamic_pointer_cast<IfcLengthMeasure>(value) != NULL)
		valueObject["value"] = dynamic_pointer_cast<IfcLengthMeasure>(value)->m_value;
	else if (dynamic_pointer_cast<IfcPlaneAngleMeasure>(value) != NULL)
		valueObject["value"] = dynamic_pointer_cast<IfcPlaneAngleMeasure>(value)->m_value;
	else if (dynamic_pointer_cast<IfcPositiveLengthMeasure>(value) != NULL)
		valueObject["value"] = dynamic_pointer_cast<IfcPositiveLengthMeasure>(value)->m_value;
	else if (dynamic_pointer_cast<IfcReal>(value) != NULL)
		valueObject["value"] = dynamic_pointer_cast<IfcReal>(value)->m_value;
	else if (dynamic_pointer_cast<IfcVolumeMeasure>(value) != NULL)
		valueObject["value"] = dynamic_pointer_cast<IfcVolumeMeasure>(value)->m_value;
	else
		valueObject["value"] = std::string("unknown : ") + std::string(value->className());
}

#ifdef TMPTEST
size_t IfcppLoader::getAttributeTypeCount()
{
	return attributeTypes.size();
}
std::string IfcppLoader::getAttributeType(size_t i)
{
	return attributeTypes[i];
}
#endif

IfcLoader* createIfcLoader()
{
	return new IfcppLoader;
}

void destroyIfcLoader(IfcLoader* aLoader)
{
	delete static_cast<IfcppLoader*>(aLoader);
}
/*
void convertToOpenGL(const std::map<int, shared_ptr<ProductShapeData> >& map_shape_data)
{
	progressTextCallback(L"Converting geometry to OpenGL format ...");
	progressValueCallback(0, "scenegraph");
	m_map_entity_id_to_switch.clear();
	m_map_representation_id_to_switch.clear();
	m_vec_existing_statesets.clear();

	shared_ptr<ProductShapeData> ifc_project_data;
	std::vector<shared_ptr<ProductShapeData> > vec_products;

	for (auto it = map_shape_data.begin(); it != map_shape_data.end(); ++it)
	{
		shared_ptr<ProductShapeData> shape_data = it->second;
		if (shape_data)
		{
			vec_products.push_back(shape_data);
		}
	}

	// create geometry for for each IfcProduct independently, spatial structure will be resolved later
	//std::map<int, osg::ref_ptr<osg::Switch> >* map_entity_id = &m_map_entity_id_to_switch;
	//std::map<int, osg::ref_ptr<osg::Switch> >* map_representations = &m_map_representation_id_to_switch;
	const int num_products = (int)vec_products.size();

#ifdef ENABLE_OPENMP
	Mutex writelock_map;
	Mutex writelock_message_callback;
	Mutex writelock_ifc_project;

#pragma omp parallel firstprivate(num_products) shared(map_entity_id, map_representations)
	{
		// time for one product may vary significantly, so schedule not so many
#pragma omp for schedule(dynamic,40)
#endif
		for (int i = 0; i < num_products; ++i)
		{
			shared_ptr<ProductShapeData>& shape_data = vec_products[i];

			weak_ptr<IfcObjectDefinition>& ifc_object_def_weak = shape_data->m_ifc_object_definition;
			if (ifc_object_def_weak.expired())
			{
				continue;
			}
			shared_ptr<IfcObjectDefinition> ifc_object_def(ifc_object_def_weak);

			std::stringstream thread_err;
			if (dynamic_pointer_cast<IfcFeatureElementSubtraction>(ifc_object_def))
			{
				// geometry will be created in method subtractOpenings
				continue;
			}
			else if (dynamic_pointer_cast<IfcProject>(ifc_object_def))
			{
				ifc_project_data = shape_data;
			}

			shared_ptr<IfcProduct> ifc_product = dynamic_pointer_cast<IfcProduct>(ifc_object_def);
			if (!ifc_product)
			{
				continue;
			}

			if (!ifc_product->m_Representation)
			{
				continue;
			}

			const int product_id = ifc_product->m_entity_id;
			//std::map<int, osg::ref_ptr<osg::Switch> > map_representation_switches;
			try
			{
				//convertProductShapeToOSG(shape_data, map_representation_switches);
				convertProductShapeToOpenGL(shape_data);
			}
			catch (OutOfMemoryException& e)
			{
				throw e;
			}
			catch (BuildingException& e)
			{
				thread_err << e.what();
			}
			catch (carve::exception& e)
			{
				thread_err << e.str();
			}
			catch (std::exception& e)
			{
				thread_err << e.what();
			}
			catch (...)
			{
				thread_err << "undefined error, product id " << product_id;
			}
//			
			if (map_representation_switches.size() > 0)
			{
				osg::ref_ptr<osg::Switch> product_switch = new osg::Switch();

				osg::ref_ptr<osg::MatrixTransform> product_transform = new osg::MatrixTransform();
				product_transform->setMatrix(convertMatrixToOSG(shape_data->getTransform()));
				product_switch->addChild(product_transform);

				std::stringstream strs_product_switch_name;
				strs_product_switch_name << "#" << product_id << "=" << ifc_product->className() << " group";
				product_switch->setName(strs_product_switch_name.str().c_str());

				for (auto it_map = map_representation_switches.begin(); it_map != map_representation_switches.end(); ++it_map)
				{
					osg::ref_ptr<osg::Switch>& repres_switch = it_map->second;
					product_transform->addChild(repres_switch);
				}

				// apply statesets if there are any
				const std::vector<shared_ptr<AppearanceData> >& vec_product_appearances = shape_data->getAppearances();
				if (vec_product_appearances.size() > 0)
				{
					applyAppearancesToGroup(vec_product_appearances, product_switch);
				}

#ifdef ENABLE_OPENMP
				ScopedLock scoped_lock(writelock_map);
#endif
				map_entity_id->insert(std::make_pair(product_id, product_switch));
				map_representations->insert(map_representation_switches.begin(), map_representation_switches.end());
			}

			if (thread_err.tellp() > 0)
			{
#ifdef ENABLE_OPENMP
				ScopedLock scoped_lock(writelock_message_callback);
#endif
				messageCallback(thread_err.str().c_str(), StatusCallback::MESSAGE_TYPE_ERROR, __FUNC__);
			}

			// progress callback
			double progress = (double)i / (double)num_products;
			if (progress - m_recent_progress > 0.02)
			{
#ifdef ENABLE_OPENMP
				if (omp_get_thread_num() == 0)
#endif
				{
					// leave 10% of progress to openscenegraph internals
					progressValueCallback(progress*0.9, "scenegraph");
					m_recent_progress = progress;
				}
			}
//
		}
#ifdef ENABLE_OPENMP
	} // implicit barrier
#endif

	try
	{
		// now resolve spatial structure
		if (ifc_project_data)
		{
			resolveProjectStructure(ifc_project_data, parent_group);
		}
	}
	catch (OutOfMemoryException& e)
	{
		throw e;
	}
	catch (BuildingException& e)
	{
		//messageCallback(e.what(), StatusCallback::MESSAGE_TYPE_ERROR, "");
	}
	catch (std::exception& e)
	{
		//messageCallback(e.what(), StatusCallback::MESSAGE_TYPE_ERROR, "");
	}
	catch (...)
	{
		//messageCallback("undefined error", StatusCallback::MESSAGE_TYPE_ERROR, __FUNC__);
	}
	//progressValueCallback(0.9, "scenegraph");
}
*/
/*
// caution: when using OpenMP, this method runs in parallel threads, so every write access to member variables needs a write lock
void convertProductShapeToOpenGL(shared_ptr<ProductShapeData>& product_shape)
{
	if (product_shape->m_ifc_object_definition.expired())
	{
		return;
	}

	shared_ptr<IfcObjectDefinition> ifc_object_def(product_shape->m_ifc_object_definition);
	shared_ptr<IfcProduct> ifc_product = dynamic_pointer_cast<IfcProduct>(ifc_object_def);
	if (!ifc_product)
	{
		return;
	}
	const int product_id = ifc_product->m_entity_id;
	std::stringstream strs_product_switch_name;
	strs_product_switch_name << "#" << product_id << "=" << ifc_product->className() << " group";
	bool draw_bounding_box = false;

	// create OSG objects
	std::vector<shared_ptr<RepresentationData> >& vec_product_representations = product_shape->m_vec_representations;
	for (size_t ii_representation = 0; ii_representation < vec_product_representations.size(); ++ii_representation)
	{
		const shared_ptr<RepresentationData>& product_representation_data = vec_product_representations[ii_representation];
		if (product_representation_data->m_ifc_representation.expired())
		{
			continue;
		}
		shared_ptr<IfcRepresentation> ifc_representation(product_representation_data->m_ifc_representation);
		const int representation_id = ifc_representation->m_entity_id;
		osg::ref_ptr<osg::Switch> representation_switch = new osg::Switch();

#ifdef _DEBUG
		std::stringstream strs_representation_name;
		strs_representation_name << strs_product_switch_name.str().c_str() << ", representation " << ii_representation;
		representation_switch->setName(strs_representation_name.str().c_str());
#endif

		const std::vector<shared_ptr<ItemShapeData> >& product_items = product_representation_data->m_vec_item_data;
		for (size_t i_item = 0; i_item < product_items.size(); ++i_item)
		{
			const shared_ptr<ItemShapeData>& item_shape = product_items[i_item];
			osg::ref_ptr<osg::MatrixTransform> item_group = new osg::MatrixTransform();
			if (!item_group) { throw OutOfMemoryException(__FUNC__); }

#ifdef _DEBUG
			std::stringstream strs_item_name;
			strs_item_name << strs_representation_name.str().c_str() << ", item " << i_item;
			item_group->setName(strs_item_name.str().c_str());
#endif

			// create shape for open shells
			for (size_t ii = 0; ii < item_shape->m_meshsets_open.size(); ++ii)
			{
				shared_ptr<carve::mesh::MeshSet<3> >& item_meshset = item_shape->m_meshsets_open[ii];
				CSG_Adapter::retriangulateMeshSet(item_meshset);
				osg::ref_ptr<osg::Geode> geode = new osg::Geode();
				if (!geode) { throw OutOfMemoryException(__FUNC__); }
				drawMeshSet(item_meshset, geode, m_geom_settings->getCoplanarFacesMaxDeltaAngle());

				if (m_geom_settings->getRenderCreaseEdges())
				{
					renderMeshsetCreaseEdges(item_meshset, geode, m_geom_settings->getCreaseEdgesMaxDeltaAngle());
				}

				// disable back face culling for open meshes
				geode->getOrCreateStateSet()->setAttributeAndModes(m_cull_back_off.get(), osg::StateAttribute::OFF);
				item_group->addChild(geode);

				if (draw_bounding_box)
				{
					carve::geom::aabb<3> bbox = item_meshset->getAABB();
					osg::ref_ptr<osg::Geometry> bbox_geom = new osg::Geometry();
					drawBoundingBox(bbox, bbox_geom);
					geode->addDrawable(bbox_geom);
				}

#ifdef _DEBUG
				std::stringstream strs_item_meshset_name;
				strs_item_meshset_name << strs_item_name.str().c_str() << ", open meshset " << ii;
				geode->setName(strs_item_meshset_name.str().c_str());
#endif
			}

			// create shape for meshsets
			for (size_t ii = 0; ii < item_shape->m_meshsets.size(); ++ii)
			{
				shared_ptr<carve::mesh::MeshSet<3> >& item_meshset = item_shape->m_meshsets[ii];
				CSG_Adapter::retriangulateMeshSet(item_meshset);
				osg::ref_ptr<osg::Geode> geode_meshset = new osg::Geode();
				if (!geode_meshset) { throw OutOfMemoryException(__FUNC__); }
				drawMeshSet(item_meshset, geode_meshset, m_geom_settings->getCoplanarFacesMaxDeltaAngle());
				item_group->addChild(geode_meshset);

				if (m_geom_settings->getRenderCreaseEdges())
				{
					renderMeshsetCreaseEdges(item_meshset, geode_meshset, m_geom_settings->getCreaseEdgesMaxDeltaAngle());
				}

				if (draw_bounding_box)
				{
					carve::geom::aabb<3> bbox = item_meshset->getAABB();
					osg::ref_ptr<osg::Geometry> bbox_geom = new osg::Geometry();
					drawBoundingBox(bbox, bbox_geom);
					geode_meshset->addDrawable(bbox_geom);
				}

#ifdef _DEBUG
				std::stringstream strs_item_meshset_name;
				strs_item_meshset_name << strs_item_name.str().c_str() << ", meshset " << ii;
				geode_meshset->setName(strs_item_meshset_name.str().c_str());
#endif
			}

			// create shape for points
			const std::vector<shared_ptr<carve::input::VertexData> >& vertex_points = item_shape->getVertexPoints();
			for (size_t ii = 0; ii < vertex_points.size(); ++ii)
			{
				const shared_ptr<carve::input::VertexData>& pointset_data = vertex_points[ii];
				if (pointset_data)
				{
					if (pointset_data->points.size() > 0)
					{
						osg::ref_ptr<osg::Geode> geode = new osg::Geode();
						if (!geode) { throw OutOfMemoryException(__FUNC__); }

						osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
						for (size_t i_pointset_point = 0; i_pointset_point < pointset_data->points.size(); ++i_pointset_point)
						{
							vec3& carve_point = pointset_data->points[i_pointset_point];
							vertices->push_back(osg::Vec3d(carve_point.x, carve_point.y, carve_point.z));
						}

						osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
						geometry->setVertexArray(vertices);
						geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size()));
						geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
						geode->getOrCreateStateSet()->setAttribute(new osg::Point(3.0f), osg::StateAttribute::ON);
						geode->addDrawable(geometry);
						geode->setCullingActive(false);
						item_group->addChild(geode);

#ifdef _DEBUG
						std::stringstream strs_item_meshset_name;
						strs_item_meshset_name << strs_item_name.str().c_str() << ", vertex_point " << ii;
						geode->setName(strs_item_meshset_name.str().c_str());
#endif
					}
				}
			}

			// create shape for polylines
			for (size_t ii = 0; ii < item_shape->m_polylines.size(); ++ii)
			{
				shared_ptr<carve::input::PolylineSetData>& polyline_data = item_shape->m_polylines[ii];
				osg::ref_ptr<osg::Geode> geode = new osg::Geode();
				if (!geode) { throw OutOfMemoryException(__FUNC__); }
				geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
				drawPolyline(polyline_data.get(), geode);
				item_group->addChild(geode);

#ifdef _DEBUG
				std::stringstream strs_item_meshset_name;
				strs_item_meshset_name << strs_item_name.str().c_str() << ", polylines " << ii;
				geode->setName(strs_item_meshset_name.str().c_str());
#endif
			}

			if (m_geom_settings->isShowTextLiterals())
			{
				for (size_t ii = 0; ii < item_shape->m_vec_text_literals.size(); ++ii)
				{
					shared_ptr<TextItemData>& text_data = item_shape->m_vec_text_literals[ii];
					if (!text_data)
					{
						continue;
					}
					carve::math::Matrix& text_pos = text_data->m_text_position;
					// TODO: handle rotation

					std::string text_str;
					text_str.assign(text_data->m_text.begin(), text_data->m_text.end());

					osg::Vec3 pos2(text_pos._41, text_pos._42, text_pos._43);

					osg::ref_ptr<osgText::Text> txt = new osgText::Text();
					if (!txt)
					{
						throw OutOfMemoryException(__FUNC__);
					}
					txt->setFont("fonts/arial.ttf");
					txt->setColor(osg::Vec4f(0, 0, 0, 1));
					txt->setCharacterSize(0.1f);
					txt->setAutoRotateToScreen(true);
					txt->setPosition(pos2);
					txt->setText(text_str.c_str());
					txt->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

					osg::ref_ptr<osg::Geode> geode = new osg::Geode();
					if (!geode) { throw OutOfMemoryException(__FUNC__); }
					geode->addDrawable(txt);
					item_group->addChild(geode);
				}
			}

			// apply statesets if there are any
			if (item_shape->m_vec_item_appearances.size() > 0)
			{
				applyAppearancesToGroup(item_shape->m_vec_item_appearances, item_group);
			}

			// If anything has been created, add it to the representation group
			if (item_group->getNumChildren() > 0)
			{
#ifdef _DEBUG
				if (item_group->getNumParents() > 0)
				{
					std::cout << __FUNC__ << ": item_group->getNumParents() > 0" << std::endl;
				}
#endif
				representation_switch->addChild(item_group);
			}
		}

		// apply statesets if there are any
		if (product_representation_data->m_vec_representation_appearances.size() > 0)
		{
			applyAppearancesToGroup(product_representation_data->m_vec_representation_appearances, representation_switch);
		}

		// If anything has been created, add it to the product group
		if (representation_switch->getNumChildren() > 0)
		{
#ifdef _DEBUG
			if (representation_switch->getNumParents() > 0)
			{
				std::cout << __FUNC__ << ": product_representation_switch->getNumParents() > 0" << std::endl;
			}
#endif
			// enable transparency for certain objects
			if (dynamic_pointer_cast<IfcSpace>(ifc_product))
			{
				representation_switch->setStateSet(m_glass_stateset);
			}
			else if (dynamic_pointer_cast<IfcCurtainWall>(ifc_product) || dynamic_pointer_cast<IfcWindow>(ifc_product))
			{
				representation_switch->setStateSet(m_glass_stateset);
				SceneGraphUtils::setMaterialAlpha(representation_switch, 0.6f);
			}

			map_representation_switches.insert(std::make_pair(representation_id, representation_switch));
		}
	}

	// TODO: if no color or material is given, set color 231/219/169 for walls, 140/140/140 for slabs 
}
*/