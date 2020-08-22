#include "Scene.h"

namespace star {

    glm::vec3 transformPoint(const glm::vec3& point, const glm::mat4& inMat);

    Mesh::Mesh()
    {
        mBvh = new accel::Bvh();
    }

    Mesh::~Mesh()
    {
        delete mBvh;
    }

    void Mesh::buildBvh()
    {
        int numTris = mVertices.size() / 3;
        std::vector<accel::BBox> bounds(numTris);

        for (int i = 0; i < numTris; ++i)
        {
            glm::vec3 v1 = mVertices[i * 3 + 0];
            glm::vec3 v2 = mVertices[i * 3 + 1];
            glm::vec3 v3 = mVertices[i * 3 + 2];

            bounds[i].grow(v1);
            bounds[i].grow(v2);
            bounds[i].grow(v3);
        }

        mBvh->build(&bounds[0], numTris);
    }

    Scene::Scene()
    {
        mBvh = new accel::Bvh();
    }

    Scene::~Scene()
    {
        delete mBvh;
        for (int i = 0; i < mMeshs.size(); ++i)
        {
            if(mMeshs[i])
                delete mMeshs[i];
        }
    }

    int Scene::findMesh(Mesh *mesh)
    {
        for (int i = 0; i < mMeshs.size(); ++i)
        {
            if(mesh == mMeshs[i])
                return i;
        }
        return -1;
    }

    void Scene::addMesh(Mesh *mesh)
    {
        mMeshs.push_back(mesh);
    }

    void Scene::addMeshInstance(const MeshInstance &instance)
    {
        int idx = findMesh(instance.mesh);
        if(idx >= 0)
        {
            MeshInstance inst = instance;
            inst.meshIdx = idx;
            mMeshInstances.push_back(inst);
        }
    }

    void Scene::createAccelerationStructures()
    {
        createBLAS();
        createTLAS();
        std::vector<accel::Bvh*> bvhs;
        std::vector<accel::BvhInstance> bvhInstances;
        for (int i = 0; i < mMeshs.size(); ++i)
        {
            bvhs.push_back(mMeshs[i]->mBvh);
        }
        for (int i = 0; i < mMeshInstances.size(); ++i)
        {
            accel::BvhInstance bvhInstance;
            bvhInstance.bvhIdx = mMeshInstances[i].meshIdx;
            bvhInstance.transform = mMeshInstances[i].transform;
            bvhInstances.push_back(bvhInstance);
        }
        mBvhTranslator.process(mBvh, bvhs, bvhInstances);

        for (int i = 0; i < mMeshInstances.size(); ++i)
        {
            Mesh* mesh = mMeshInstances[i].mesh;
            SceneObject sceneObject;
            sceneObject.transform = mMeshInstances[i].transform;
            sceneObject.albedo = mesh->mAlbedo;
            sceneObject.emission = mesh->mEmission;
            sceneObject.matParams = glm::vec4(mesh->mMetallic, mesh->mRoughness, 0.0f, 0.0f);
            mSceneObjects.push_back(sceneObject);
        }

        int verticesCount = 0;
        for (int i = 0; i < mMeshs.size(); i++)
        {
            int numIndices = mMeshs[i]->mBvh->getNumIndices();
            uint32_t* triIndices = mMeshs[i]->mBvh->getIndices();

            for (int j = 0; j < numIndices; j++)
            {
                int index = triIndices[j];
                int v1 = (index * 3 + 0) + verticesCount;
                int v2 = (index * 3 + 1) + verticesCount;
                int v3 = (index * 3 + 2) + verticesCount;
                mIndices.push_back({ v1, v2, v3 });
            }

            for (int j = 0; j < mMeshs[i]->mVertices.size(); ++j)
            {
                Vertex vertex;
                vertex.position = mMeshs[i]->mVertices[j];//glm::vec3(mMeshs[i]->mVertices[j].x, mMeshs[i]->mVertices[j].y, mMeshs[i]->mVertices[j].z);
                vertex.normal = glm::vec3(mMeshs[i]->mNormals[j].x, mMeshs[i]->mNormals[j].y, mMeshs[i]->mNormals[j].z);
                vertex.uv = glm::vec2(mMeshs[i]->mUVs[j].x, mMeshs[i]->mUVs[j].y);
                mVertices.push_back(vertex);
            }

            verticesCount += mMeshs[i]->mVertices.size();
        }
    }

    void Scene::createTLAS()
    {
        std::vector<accel::BBox> bounds;
        bounds.resize(mMeshInstances.size());

        for (int i = 0; i < mMeshInstances.size(); i++)
        {
            accel::BBox bbox = mMeshs[mMeshInstances[i].meshIdx]->mBvh->getBound();

            glm::mat4 matrix = mMeshInstances[i].transform;

            glm::vec3 minBound = bbox.mMin;
            glm::vec3 maxBound = bbox.mMax;

            glm::vec3 right       = glm::vec3(matrix[0][0], matrix[0][1], matrix[0][2]);
            glm::vec3 up          = glm::vec3(matrix[1][0], matrix[1][1], matrix[1][2]);
            glm::vec3 forward     = glm::vec3(matrix[2][0], matrix[2][1], matrix[2][2]);
            glm::vec3 translation = glm::vec3(matrix[3][0], matrix[3][1], matrix[3][2]);

            glm::vec3 xa = right * minBound.x;
            glm::vec3 xb = right * maxBound.x;

            glm::vec3 ya = up * minBound.y;
            glm::vec3 yb = up * maxBound.y;

            glm::vec3 za = forward * minBound.z;
            glm::vec3 zb = forward * maxBound.z;

            minBound = glm::min(xa, xb) + glm::min(ya, yb) + glm::min(za, zb) + translation;
            maxBound = glm::max(xa, xb) + glm::max(ya, yb) + glm::max(za, zb) + translation;

            accel::BBox bound;
            bound.mMin = minBound;
            bound.mMax = maxBound;

            bounds[i] = bound;
        }
        mBvh->build(&bounds[0], bounds.size());
    }

    void Scene::createBLAS()
    {
        for (int i = 0; i < mMeshs.size(); ++i)
        {
            mMeshs[i]->buildBvh();
        }
    }

    glm::vec3 transformPoint(const glm::vec3& point, const glm::mat4& inMat)
    {
        glm::mat4 mat = glm::transpose(inMat);
        float x = point.x, y = point.y, z = point.z;
        float xp = mat[0][0] * x + mat[0][1] * y + mat[0][2] * z + mat[0][3];
        float yp = mat[1][0] * x + mat[1][1] * y + mat[1][2] * z + mat[1][3];
        float zp = mat[2][0] * x + mat[2][1] * y + mat[2][2] * z + mat[2][3];
        float wp = mat[3][0] * x + mat[3][1] * y + mat[3][2] * z + mat[3][3];
        assert(wp != 0);

        if (wp == 1.0f)
            return glm::vec3(xp, yp, zp);
        else
            return glm::vec3(xp, yp, zp) / wp;
    }
}
