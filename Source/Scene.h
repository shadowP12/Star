#ifndef STAR_SCENE_H
#define STAR_SCENE_H
#include "Accelerator/Bvh.h"
#include "Accelerator/BvhTranslator.h"
#include <glm/glm.hpp>
#include <vector>
namespace star {
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
        accel::Bvh* mBvh = nullptr;
        accel::BvhTranslator mBvhTranslator;
        std::vector<Mesh*> mMeshs;
        std::vector<MeshInstance> mMeshInstances;
        std::vector<glm::ivec3> mVertIndices;
        std::vector<glm::vec3> mVertices;
        std::vector<glm::vec3> mNormals;
        std::vector<glm::vec2> mUV;
        std::vector<glm::mat4> mTransforms;
    };
}


#endif
