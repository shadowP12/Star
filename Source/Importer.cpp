#include "Importer.h"
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include "CommonMath.h"
#include "TriangleMesh.h"

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
                printf("index component type not supported!\n");
                return nullptr;
        }

        // load material
        std::shared_ptr<Material> mat = std::shared_ptr<Material>(new Material());

        mat->emissive = glm::vec3(primitive->material->emissive_factor[0], primitive->material->emissive_factor[1], primitive->material->emissive_factor[2]);
        mat->baseColor = glm::vec3(primitive->material->pbr_metallic_roughness.base_color_factor[0],
                                   primitive->material->pbr_metallic_roughness.base_color_factor[1],
                                   primitive->material->pbr_metallic_roughness.base_color_factor[2]);
        mat->metallic = primitive->material->pbr_metallic_roughness.metallic_factor;
        mat->roughness = primitive->material->pbr_metallic_roughness.roughness_factor;

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

SceneData* loadScene(std::string path)
{
    SceneData* sceneData = new SceneData();
    cgltf_options options = {static_cast<cgltf_file_type>(0)};
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
    if (result == cgltf_result_success)
        result = cgltf_load_buffers(&options, data, path.c_str());

    if (result == cgltf_result_success)
        result = cgltf_validate(data);

    // load meshs
    for (int i = 0; i < data->nodes_count; i++)
    {
        if(data->nodes[i].mesh != nullptr)
        {
            std::shared_ptr<TriangleMesh> triangleMesh = loadMesh(data->nodes[i].mesh, sceneData->materials);
            triangleMesh->mWorldMatrix = getWorldMatrix(&data->nodes[i]);
            sceneData->triangleMeshs.push_back(triangleMesh);
        }
    }

    for (int i = 0; i < sceneData->triangleMeshs.size(); i++)
    {
        for (int j = 0; j < sceneData->triangleMeshs[i]->mTriangleCount; j++)
        {
            std::shared_ptr<Triangle> tri = std::shared_ptr<Triangle>(new Triangle(sceneData->triangleMeshs[i], j, sceneData->triangleMeshs[i]->mWorldMatrix));
            sceneData->triangles.push_back(tri);
        }
    }

    cgltf_free(data);
    return sceneData;
}

void destroyScene(SceneData* data)
{
    if(data)
    {
        delete data;
    }
}