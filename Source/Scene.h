#ifndef STAR_SCENE_H
#define STAR_SCENE_H
#include "Accelerator/Bvh.h"
#include "Accelerator/BvhTranslator.h"
#include <glm/glm.hpp>
#include <vector>
namespace star {
    struct Vertex {
        alignas(16) glm::vec4 positionUVX;
        alignas(16) glm::vec4 normalUVY;
    };

    struct Indices {
        alignas(16) glm::ivec3 indices;
    };

    struct Transform {
        alignas(16) glm::mat4 transform;
    };

    class Mesh
    {
    public:
        Mesh();
        ~Mesh();
        void buildBvh();
    private:
        friend class Scene;
        friend class Importer;
        accel::Bvh* mBvh = nullptr;
        std::vector<glm::vec3> mVertices;
        std::vector<glm::vec3> mNormals;
        std::vector<glm::vec2> mUVs;
    };

    struct MeshInstance
    {
        int meshIdx = -1;
        Mesh* mesh = nullptr;
        glm::mat4 transform;
    };

    class Scene
    {
    public:
        Scene();
        ~Scene();
        void addMesh(Mesh* mesh);
        void addMeshInstance(const MeshInstance& instance);
        void createAccelerationStructures();
    private:
        int findMesh(Mesh* mesh);
        void createBLAS();
        void createTLAS();
    private:
        friend class Renderer;
        accel::Bvh* mBvh = nullptr;
        accel::BvhTranslator mBvhTranslator;
        std::vector<Mesh*> mMeshs;
        std::vector<MeshInstance> mMeshInstances;
        std::vector<Indices> mIndices;
        std::vector<Vertex> mVertices;
        std::vector<Transform> mTransforms;
    };
}


#endif
