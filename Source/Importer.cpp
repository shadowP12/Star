#include "Importer.h"
#include "Scene.h"
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
namespace star {
    // cgltf helper funcs
    int getCNodeInxFromCData(const cgltf_node* node, const cgltf_data* data);
    bool findAttributesType(cgltf_primitive* primitive, cgltf_attribute_type type);
    bool findAttributesType(cgltf_primitive* primitive, cgltf_attribute** inAtt, cgltf_attribute_type type);
    glm::mat4 getLocalMatrix(cgltf_node* node);
    glm::mat4 getWorldMatrix(cgltf_node* node);

    Importer::Importer() {}

    Importer::~Importer() {}

    ImportedResult Importer::load(std::string path)
    {
        ImportedResult result;
        cgltf_options options = {static_cast<cgltf_file_type>(0)};
        cgltf_data* data = NULL;
        cgltf_result ret = cgltf_parse_file(&options, path.c_str(), &data);
        if (ret == cgltf_result_success)
            ret = cgltf_load_buffers(&options, data, path.c_str());

        if (ret == cgltf_result_success)
            ret = cgltf_validate(data);

        std::map<cgltf_mesh*, Mesh*> meshHelper;
        for (int i = 0; i < data->meshes_count; ++i)
        {
            cgltf_mesh* cMesh = &data->meshes[i];
            std::vector<glm::vec3> positionBuffer;
            std::vector<glm::vec2> texcoordBuffer;
            std::vector<glm::vec3> normalBuffer;
            std::vector<uint32_t> indexBuffer;
            uint32_t indexStart = 0;
            uint32_t indexCount = 0;

            for (int i = 0; i < cMesh->primitives_count; i++)
            {
                cgltf_primitive *cPrimitive = &cMesh->primitives[i];
                indexStart += indexCount;
                indexCount = 0;

                // Vertices
                float *localPositionBuffer = nullptr;
                float *localTexcoordBuffer = nullptr;
                float *localNormalBuffer = nullptr;

                cgltf_attribute *positionAttributes = nullptr;
                assert(findAttributesType(cPrimitive, &positionAttributes, cgltf_attribute_type_position));
                cgltf_accessor *posAccessor = positionAttributes->data;
                cgltf_buffer_view *posView = posAccessor->buffer_view;
                uint8_t *posDatas = (uint8_t *) (posView->buffer->data) + posAccessor->offset + posView->offset;
                localPositionBuffer = (float *) posDatas;

                cgltf_attribute *normalAttributes = nullptr;
                if (findAttributesType(cPrimitive, &normalAttributes, cgltf_attribute_type_normal)) {
                    cgltf_accessor *normalAccessor = normalAttributes->data;
                    cgltf_buffer_view *normalView = normalAccessor->buffer_view;
                    uint8_t *normalDatas =
                            (uint8_t *) (normalView->buffer->data) + normalAccessor->offset + normalView->offset;
                    localNormalBuffer = (float *) normalDatas;
                }


                cgltf_attribute *texcoordAttributes = nullptr;
                if (findAttributesType(cPrimitive, &texcoordAttributes, cgltf_attribute_type_texcoord)) {
                    cgltf_accessor *texcoordAccessor = texcoordAttributes->data;
                    cgltf_buffer_view *texcoordView = texcoordAccessor->buffer_view;
                    uint8_t *texcoordDatas =
                            (uint8_t *) (texcoordView->buffer->data) + texcoordAccessor->offset + texcoordView->offset;
                    localTexcoordBuffer = (float *) texcoordDatas;
                }

                for (size_t v = 0; v < posAccessor->count; v++)
                {
                    positionBuffer.push_back(glm::vec3(localPositionBuffer[v * 3], localPositionBuffer[v * 3 + 1], localPositionBuffer[v * 3 + 2]));

                    if(localTexcoordBuffer)
                    {
                        texcoordBuffer.push_back(glm::vec2(localTexcoordBuffer[v * 2], localTexcoordBuffer[v * 2 + 1]));
                    } else
                    {
                        texcoordBuffer.push_back(glm::vec2(0.0f, 0.0f));
                    }

                    if(localNormalBuffer)
                    {
                        normalBuffer.push_back(glm::vec3(localNormalBuffer[v * 3], localNormalBuffer[v * 3 + 1], localNormalBuffer[v * 3 + 2]));
                    } else
                    {
                        normalBuffer.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
                    }
                }

                // Indices
                cgltf_accessor* cIndexAccessor = cPrimitive->indices;
                cgltf_buffer_view* cIndexBufferView = cIndexAccessor->buffer_view;
                cgltf_buffer * cIndexBuffer = cIndexBufferView->buffer;

                indexCount = static_cast<uint32_t>(cIndexAccessor->count);

                switch (cIndexAccessor->component_type)
                {
                    case cgltf_component_type_r_32u:
                    {
                        uint32_t *buf = new uint32_t[indexCount];
                        uint8_t *src = (uint8_t *)cIndexBuffer->data + cIndexAccessor->offset + cIndexBufferView->offset;
                        memcpy(buf, src, indexCount * sizeof(uint32_t));
                        for (size_t index = 0; index < indexCount; index++)
                        {
                            indexBuffer.push_back(buf[index]);
                        }
                        break;
                    }

                    case cgltf_component_type_r_16u:
                    {
                        uint16_t *buf = new uint16_t[indexCount];
                        uint8_t *src = (uint8_t *) cIndexBuffer->data + cIndexAccessor->offset + cIndexBufferView->offset;
                        memcpy(buf, src, indexCount * sizeof(uint16_t));
                        for (size_t index = 0; index < indexCount; index++)
                        {
                            indexBuffer.push_back(buf[index]);
                        }
                        break;
                    }
                    case cgltf_component_type_r_8u:
                    {
                        uint8_t *buf = new uint8_t[indexCount];
                        uint8_t *src = (uint8_t *) cIndexBuffer->data + cIndexAccessor->offset + cIndexBufferView->offset;
                        memcpy(buf, src, indexCount * sizeof(uint8_t));
                        for (size_t index = 0; index < indexCount; index++)
                        {
                            indexBuffer.push_back(buf[index]);
                        }
                        break;
                    }
                    default:
                        printf("Index component type not supported! \n");
                }
            }

            Mesh* mesh = new Mesh();
            for (int j = 0; j < indexBuffer.size(); ++j)
            {
                int index = indexBuffer[j];
                mesh->mVertices.push_back(positionBuffer[index]);
                mesh->mNormals.push_back(normalBuffer[index]);
                mesh->mUVs.push_back(texcoordBuffer[index]);
            }

            result.meshs.push_back(mesh);
            meshHelper[cMesh] = mesh;
        }

        for (int i = 0; i < data->nodes_count; i++)
        {
            if(data->nodes[i].mesh != nullptr)
            {
                cgltf_mesh* cMesh = data->nodes[i].mesh;
                MeshInstance meshInstance;
                meshInstance.mesh = meshHelper[cMesh];
                meshInstance.transform = getWorldMatrix(&data->nodes[i]);
                result.meshInstances.push_back(meshInstance);
            }
        }

        cgltf_free(data);

        return result;
    }

    int getCNodeInxFromCData(const cgltf_node* node, const cgltf_data* data)
    {
        for (size_t i = 0; i < data->nodes_count; ++i)
        {
            if(&data->nodes[i] == node)
            {
                return i;
            }
        }
        return -1;
    }

    bool findAttributesType(cgltf_primitive* primitive, cgltf_attribute_type type)
    {
        for (int i = 0; i < primitive->attributes_count; i++)
        {
            cgltf_attribute* att = &primitive->attributes[i];
            if(att->type == type)
            {
                return true;
            }
        }
        return false;
    }

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
}