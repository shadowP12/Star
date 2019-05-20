#include "Scene.h"
#include <iostream>
#include <assert.h>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf.h"


std::shared_ptr<TriangleMesh> loadMesh(tinygltf::Model& gltfModel, uint32_t idx)
{
	const tinygltf::Mesh mesh = gltfModel.meshes[idx];

	uint32_t indexStart = 0;
	uint32_t indexCount = 0;
	std::vector<Vertex> vertexData;
	std::vector<uint32_t> indexData;

	for (size_t j = 0; j < mesh.primitives.size(); j++)
	{
		const tinygltf::Primitive &primitive = mesh.primitives[j];
		indexStart += indexCount;
		indexCount = 0;

		// Vertices
		const float *bufferPos = nullptr;
		const float *bufferNormals = nullptr;
		const float *bufferTexCoords = nullptr;

		assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

		const tinygltf::Accessor &posAccessor = gltfModel.accessors[primitive.attributes.find("POSITION")->second];
		const tinygltf::BufferView &posView = gltfModel.bufferViews[posAccessor.bufferView];
		bufferPos = reinterpret_cast<const float *>(&(gltfModel.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));

		if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
		{
			const tinygltf::Accessor &normAccessor = gltfModel.accessors[primitive.attributes.find("NORMAL")->second];
			const tinygltf::BufferView &normView = gltfModel.bufferViews[normAccessor.bufferView];
			bufferNormals = reinterpret_cast<const float *>(&(gltfModel.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
		}

		if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
		{
			const tinygltf::Accessor &uvAccessor = gltfModel.accessors[primitive.attributes.find("TEXCOORD_0")->second];
			const tinygltf::BufferView &uvView = gltfModel.bufferViews[uvAccessor.bufferView];
			bufferTexCoords = reinterpret_cast<const float *>(&(gltfModel.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
		}

		for (size_t v = 0; v < posAccessor.count; v++)
		{
			Vertex vert{};
			vert.pos = glm::make_vec3(&bufferPos[v * 3]);
			vert.normal = bufferNormals ? glm::make_vec3(&bufferNormals[v * 3]) : glm::vec3(0.0f);
			vert.uv = bufferTexCoords ? glm::make_vec2(&bufferTexCoords[v * 2]) : glm::vec3(0.0f);
			vertexData.push_back(vert);
		}

		// Indices
		const tinygltf::Accessor &accessor = gltfModel.accessors[primitive.indices];
		const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];

		indexCount = static_cast<uint32_t>(accessor.count);

		switch (accessor.componentType)
		{
		case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
		{
			uint32_t *buf = new uint32_t[accessor.count];
			memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
			for (size_t index = 0; index < accessor.count; index++)
			{
				indexData.push_back(buf[index]);
			}
			break;
		}
		case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
		{
			uint16_t *buf = new uint16_t[accessor.count];
			memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
			for (size_t index = 0; index < accessor.count; index++)
			{
				indexData.push_back(buf[index]);
			}
			break;

		}
		case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
		{
			uint8_t *buf = new uint8_t[accessor.count];
			memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
			for (size_t index = 0; index < accessor.count; index++)
			{
				indexData.push_back(buf[index]);
			}
			break;
		}
		default:
			std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
			return nullptr;
		}
	}//primitives
	std::shared_ptr<TriangleMesh> triangleMesh = std::shared_ptr<TriangleMesh>(new TriangleMesh());
	triangleMesh->mVertexBuffer = vertexData;
	triangleMesh->mIndexBuffer = indexData;
	triangleMesh->mVertexCount = vertexData.size();
	triangleMesh->mIndexCount = indexData.size();
	triangleMesh->mTriangleCount = indexData.size() / 3;

	return triangleMesh;
}


Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::load(std::string file)
{
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF gltfContext;
	std::string error;

	bool fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, file.c_str());

	if (!fileLoaded)
	{
		std::cout << error << std::endl;
		return;
	}

	for (size_t i = 0; i < gltfModel.nodes.size(); i++)
	{
		//还有各个node之间的层次关系未实现
		const tinygltf::Node node = gltfModel.nodes[i];
		if (node.mesh <= -1)
		{
			continue;
		}
		std::shared_ptr<TriangleMesh> triangleMesh = loadMesh(gltfModel,node.mesh);
		std::shared_ptr<Model> model = std::shared_ptr<Model>(new Model(node.name,triangleMesh));
		model->initModel();
		mModels.push_back(model);
	}
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

std::vector<std::shared_ptr<Primitive>>& Scene::getPrimitives()
{
	return mPrimitives;
}

void Scene::draw(std::shared_ptr<ShaderProgram> sp)
{
	for (int i = 0; i < mModels.size(); i++)
	{
		sp->setMat4("model",mModels[i]->getWorldMatrix());
		mModels[i]->draw();
	}
}
