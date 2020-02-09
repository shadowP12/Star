#include "Scene.h"
#include <iostream>
#include <assert.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <stdio.h>

bool findAttributesType(cgltf_primitive* primitive, cgltf_attribute** inAtt, cgltf_attribute_type type)
{
    for (int i = 0; i < primitive->attributes_count; i++)
    {
        cgltf_attribute* att = &primitive->attributes[i];
        if(att->type == type)
        {
            *inAtt = att;
            return true;
        }
    }
    return false;
}

std::shared_ptr<TriangleMesh> loadMesh(cgltf_mesh* mesh, std::vector<std::shared_ptr<Material>>& materials)
{
	uint32_t indexStart = 0;
	uint32_t indexCount = 0;
	std::vector<Vertex> vertexData;
	std::vector<uint32_t> indexData;
    std::shared_ptr<TriangleMesh> triangleMesh = std::shared_ptr<TriangleMesh>(new TriangleMesh());

	for (int i = 0; i < mesh->primitives_count; i++)
	{
        cgltf_primitive* primitive = &mesh->primitives[i];
		indexStart += indexCount;
		indexCount = 0;

		// Vertices
		float *bufferPos = nullptr;
		float *bufferNormals = nullptr;
		float *bufferTexCoords = nullptr;

        cgltf_attribute* positionAttributes = nullptr;
		assert(findAttributesType(primitive, &positionAttributes, cgltf_attribute_type_position));

		cgltf_accessor* posAccessor = positionAttributes->data;
        cgltf_buffer_view* posView = posAccessor->buffer_view;
        uint8_t* posData = (uint8_t*)(posView->buffer->data) + posAccessor->offset + posView->offset;
        bufferPos = (float*)posData;

        cgltf_attribute* normalAttributes = nullptr;
		if (findAttributesType(primitive, &normalAttributes, cgltf_attribute_type_normal))
		{
            cgltf_accessor* normalAccessor = normalAttributes->data;
            cgltf_buffer_view* normalView = normalAccessor->buffer_view;
            uint8_t* normalData = (uint8_t*)(normalView->buffer->data) + normalAccessor->offset + normalView->offset;
			bufferNormals = (float*)normalData;
		}
        cgltf_attribute* texcoordAttributes = nullptr;
		if (findAttributesType(primitive, &texcoordAttributes, cgltf_attribute_type_texcoord))
		{
            cgltf_accessor* texcoordAccessor = texcoordAttributes->data;
            cgltf_buffer_view* texcoordView = texcoordAccessor->buffer_view;
            uint8_t* texcoordData = (uint8_t*)(texcoordView->buffer->data) + texcoordAccessor->offset + texcoordView->offset;
			bufferTexCoords = (float*)texcoordData;
		}

		for (size_t v = 0; v < posAccessor->count; v++)
		{
			Vertex vert{};
			vert.pos = glm::make_vec3(&bufferPos[v * 3]);
			vert.normal = bufferNormals ? glm::make_vec3(&bufferNormals[v * 3]) : glm::vec3(0.0f);
			vert.uv = bufferTexCoords ? glm::make_vec2(&bufferTexCoords[v * 2]) : glm::vec3(0.0f);
			vertexData.push_back(vert);
		}

		// Indices
        cgltf_accessor* indexAccessor = primitive->indices;
		cgltf_buffer_view* indexBufferView = indexAccessor->buffer_view;
		cgltf_buffer* indexBuffer = indexBufferView->buffer;

		indexCount = static_cast<uint32_t>(indexAccessor->count);

		switch (indexAccessor->component_type)
		{
		case cgltf_component_type_r_32u:
		{
			uint32_t* buf = new uint32_t[indexCount];
			uint8_t* src = (uint8_t*)indexBuffer->data + indexAccessor->offset + indexBufferView->offset;
			memcpy(buf, src, indexCount * sizeof(uint32_t));
			for (size_t index = 0; index < indexCount; index++)
			{
				indexData.push_back(buf[index]);
			}
			break;
		}

		case cgltf_component_type_r_16u:
		{
			uint16_t *buf = new uint16_t[indexCount];
            uint8_t* src = (uint8_t*)indexBuffer->data + indexAccessor->offset + indexBufferView->offset;
			memcpy(buf, src, indexCount * sizeof(uint16_t));
			for (size_t index = 0; index < indexCount; index++)
			{
				indexData.push_back(buf[index]);
			}
			break;
		}
		case cgltf_component_type_r_8u:
		{
			uint8_t *buf = new uint8_t[indexCount];
            uint8_t* src = (uint8_t*)indexBuffer->data + indexAccessor->offset + indexBufferView->offset;
			memcpy(buf, src, indexCount * sizeof(uint8_t));
			for (size_t index = 0; index < indexCount; index++)
			{
				indexData.push_back(buf[index]);
			}
			break;
		}
		default:
			std::cerr << "Index component type " << indexAccessor->component_type << " not supported!" << std::endl;
			return nullptr;
		}

		// load material
		std::shared_ptr<Material> mat = std::shared_ptr<Material>(new Material());

		mat->emissive = glm::vec3(primitive->material->emissive_factor[0], primitive->material->emissive_factor[1], primitive->material->emissive_factor[2]);
		mat->baseColor = glm::vec3(primitive->material->pbr_metallic_roughness.base_color_factor[0],
                                   primitive->material->pbr_metallic_roughness.base_color_factor[1],
                                   primitive->material->pbr_metallic_roughness.base_color_factor[2]);
		materials.push_back(mat);
		TriangleSubMesh subMesh;
        subMesh.materialID = materials.size() - 1;
        subMesh.indexCount = indexCount;
        subMesh.indexOffset = indexStart;
        triangleMesh->mSubMeshs.push_back(subMesh);
	}//primitives

	triangleMesh->mVertexBuffer = vertexData;
	triangleMesh->mIndexBuffer = indexData;
	triangleMesh->mVertexCount = vertexData.size();
	triangleMesh->mIndexCount = indexData.size();
	triangleMesh->mTriangleCount = indexData.size() / 3;

	return triangleMesh;
}

glm::mat4 getLocalMatrix(cgltf_node* node)
{
    glm::vec3 translation = glm::vec3(0.0f);
    if (node->has_translation)
    {
        translation.x = node->translation[0];
        translation.y = node->translation[1];
        translation.z = node->translation[2];
    }

    glm::quat rotation = glm::quat(1, 0, 0, 0);
    if (node->has_rotation)
    {
        rotation.x = node->rotation[0];
        rotation.y = node->rotation[1];
        rotation.z = node->rotation[2];
        rotation.w = node->rotation[3];
    }

    glm::vec3 scale = glm::vec3(1.0f);
    if (node->has_scale)
    {
        scale.x = node->scale[0];
        scale.y = node->scale[1];
        scale.z = node->scale[2];
    }

    glm::mat4 r, t, s;
    r = glm::toMat4(rotation);
    t = glm::translate(glm::mat4(1.0), translation);
    s = glm::scale(glm::mat4(1.0), scale);
    return t * r * s;
}

glm::mat4 getWorldMatrix(cgltf_node* node)
{
    cgltf_node* curNode = node;
    glm::mat4 out = getLocalMatrix(curNode);

    while (curNode->parent != nullptr)
    {
        curNode = node->parent;
        out = getLocalMatrix(curNode) * out;
    }
    return out;
}

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::load(std::string file)
{
    cgltf_options options = {static_cast<cgltf_file_type>(0)};
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, file.c_str(), &data);
    if (result == cgltf_result_success)
        result = cgltf_load_buffers(&options, data, file.c_str());

    if (result == cgltf_result_success)
        result = cgltf_validate(data);

    printf("Result: %d\n", result);

    // load meshs
    for (int i = 0; i < data->nodes_count; i++)
    {
        if(data->nodes[i].mesh != nullptr)
        {
            std::shared_ptr<TriangleMesh> triangleMesh = loadMesh(data->nodes[i].mesh, mMaterials);
            triangleMesh->mWorldMatrix = getWorldMatrix(&data->nodes[i]);
            std::shared_ptr<Model> model = std::shared_ptr<Model>(new Model(data->nodes[i].name, triangleMesh));
            model->initModel();
            mModels.push_back(model);
        }
    }

    cgltf_free(data);
//
//
//	tinygltf::Model gltfModel;
//	tinygltf::TinyGLTF gltfContext;
//	std::string error;
//    std::string warn;
//	bool fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warn, file.c_str());
//
//	if (!fileLoaded)
//	{
//		std::cout << error << std::endl;
//		return;
//	}
//    // load node
//    for (int i = 0; i < gltfModel.nodes.size(); i++)
//    {
//        const tinygltf::Node& gltfNode = gltfModel.nodes[i];
//        SceneNode node;
//        node.id = i;
//        node.parent = -1;
//        glm::vec3 translation = glm::vec3(0.0f);
//        if (gltfNode.translation.size() == 3)
//        {
//            translation = glm::make_vec3(gltfNode.translation.data());
//        }
//
//        glm::quat rotation = glm::quat(1, 0, 0, 0);
//        if (gltfNode.rotation.size() == 4)
//        {
//            rotation = glm::make_quat(gltfNode.rotation.data());
//        }
//
//        glm::vec3 scale = glm::vec3(1.0f);
//        if (gltfNode.scale.size() == 3)
//        {
//            scale = glm::make_vec3(gltfNode.scale.data());
//        }
//
//        node.translation = translation;
//        node.scale = scale;
//        node.rotation = rotation;
//        mNodes.push_back(node);
//    }
//
//    for (int i = 0; i < gltfModel.nodes.size(); i++)
//    {
//        const tinygltf::Node& gltfNode = gltfModel.nodes[i];
//        SceneNode& node = mNodes[i];
//        for (int j = 0; j < gltfNode.children.size(); j++)
//        {
//            SceneNode& childrenNode = mNodes[gltfNode.children[j]];
//            childrenNode.parent = node.id;
//            node.childrens.push_back(childrenNode.id);
//        }
//    }
//
//    // load material
//    for (size_t i = 0; i < gltfModel.materials.size(); i++)
//    {
//        const tinygltf::Material gltfMaterial = gltfModel.materials[i];
//        std::shared_ptr<Material> material = std::shared_ptr<Material>(new Material());
//        tinygltf::PbrMetallicRoughness values = gltfMaterial.pbrMetallicRoughness;
//        material->baseColor = glm::vec3(values.baseColorFactor[0], values.baseColorFactor[1], values.baseColorFactor[2]);
//        material->emissive = glm::vec3(gltfMaterial.emissiveFactor[0], gltfMaterial.emissiveFactor[1],gltfMaterial.emissiveFactor[2]);
//        mMaterials.push_back(material);
//    }
//
//	// load mesh
//	for (size_t i = 0; i < gltfModel.nodes.size(); i++)
//	{
//		const tinygltf::Node node = gltfModel.nodes[i];
//		if (node.mesh <= -1)
//		{
//			continue;
//		}
//		std::shared_ptr<TriangleMesh> triangleMesh = loadMesh(gltfModel,node.mesh);
//		triangleMesh->mWorldMatrix = getWorldMatrix(i);
//		std::shared_ptr<Model> model = std::shared_ptr<Model>(new Model(node.name,triangleMesh));
//		model->initModel();
//		mModels.push_back(model);
//	}
}

void Scene::genPrimitives()
{
	for (int i = 0; i < mModels.size(); i++)
	{
		for (int j = 0; j < mModels[i]->getMesh()->mTriangleCount; j++)
		{
			std::shared_ptr<Triangle> tri = std::shared_ptr<Triangle>(new Triangle(mModels[i]->getMesh(),j,mModels[i]->getWorldMatrix()));
			std::shared_ptr<Primitive> primitive = std::shared_ptr<Primitive>(new Primitive(tri));
			mPrimitives.push_back(primitive);
		}
	}
}

void Scene::draw(std::shared_ptr<ShaderProgram> sp)
{
	for (int i = 0; i < mModels.size(); i++)
	{
		sp->setMat4("model",mModels[i]->getWorldMatrix());
		mModels[i]->draw();
	}
}

std::vector<std::shared_ptr<Primitive>>& Scene::getPrimitives()
{
    return mPrimitives;
}

std::vector<std::shared_ptr<Material>>& Scene::getMaterials()
{
    return mMaterials;
}

