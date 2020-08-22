#ifndef STAR_SCENE_H
#define STAR_SCENE_H
#include "Accelerator/Bvh.h"
#include "Accelerator/BvhTranslator.h"
#include <glm/glm.hpp>
#include <vector>
namespace star {
    struct Vertex {
        alignas(16) glm::vec3 position;
        alignas(16) glm::vec3 normal;
        alignas(4) glm::vec2 uv;
    };

    struct Index {
        alignas(4) int idx0;
        alignas(4) int idx1;
        alignas(4) int idx2;
        alignas(4) int padding;
    };

    struct SceneObject {
        alignas(16) glm::mat4 transform;
        alignas(16) glm::vec3 albedo;
        alignas(16) glm::vec3 emission;
        alignas(16) glm::vec4 matParams;
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
        glm::vec3 mAlbedo;
        glm::vec3 mEmission;
        float mMetallic;
        float mRoughness;
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
        std::vector<Index> mIndices;
        std::vector<Vertex> mVertices;
        std::vector<SceneObject> mSceneObjects;
    };
}


#endif
