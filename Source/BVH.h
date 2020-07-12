#pragma once

#include "CommonMath.h"
#include "TriangleMesh.h"

class BVHBuildNode : public std::enable_shared_from_this<BVHBuildNode>
{
public:
    BBox bound;
    std::shared_ptr<BVHBuildNode> children[2];
    int splitAxis;
    int firstPrimitiveOffset;
    int numPrimitives;
public:
    //初始化叶子节点
    void InitLeaf(int first, int num, const BBox& bound)
    {
        firstPrimitiveOffset = first;
        numPrimitives = num;
        this->bound = bound;
        children[0] = children[1] = nullptr;
    }

    //初始化中间节点
    void InitInterior(std::shared_ptr<BVHBuildNode> c1, std::shared_ptr<BVHBuildNode> c2, int axis)
    {
        children[0] = c1;
        children[1] = c2;
        bound = Union(c1->bound, c2->bound);
        splitAxis = axis;
    }
};

class BVHPrimitiveInfo
{
public:
    BVHPrimitiveInfo() {}
    BVHPrimitiveInfo(int i, const BBox& b) :index(i), bound(b), centroid(b.mMin * 0.5f + b.mMax * 0.5f)
    {
    }
public:
    int index;
    BBox bound;
    glm::vec3  centroid;
};

class LinearBVHNode
{
public:
    BBox bound;
    uint16_t numPrimitive;
    uint8_t axis;
    int primitiveOffset;//指向图元
    int secondChildOffset;
};

class BVH
{
public:
    BVH(const std::vector<std::shared_ptr<Triangle>>& primitives)
    {
        if (primitives.empty())
        {
            return;
        }
        std::vector<BVHPrimitiveInfo> primitiveInfos(primitives.size());
        for (int i = 0; i < primitives.size(); ++i)
        {
            primitiveInfos[i] = BVHPrimitiveInfo(i, primitives[i]->worldBound());
        }
        std::shared_ptr<BVHBuildNode> root;
        int totalNodes = 0;
        root = recursiveBuild(primitiveInfos, 0, primitiveInfos.size(), &totalNodes, primitives);
        mNodes = new LinearBVHNode[totalNodes];
        mNodeCount = totalNodes;
        int offset = 0;
        flattenBVHTree(root, &offset);
    }

    ~BVH() {}
    LinearBVHNode* getNodes() {return mNodes;}
    int getNodeCount() { return mNodeCount; }
    std::vector<std::shared_ptr<Triangle>> getPrims() { return mPrimitives; }
private:
    std::shared_ptr<BVHBuildNode> recursiveBuild(std::vector<BVHPrimitiveInfo>& primitiveInfos, int start, int end, int* totalNodes, const std::vector<std::shared_ptr<Triangle>>& primitives)
    {
        std::shared_ptr<BVHBuildNode> node = std::shared_ptr<BVHBuildNode>(new BVHBuildNode());
        (*totalNodes)++;
        BBox bound;
        for (int i = start; i < end; ++i)
        {
            bound = Union(bound, primitiveInfos[i].bound);
        }

        int numPrimitive = end - start;
        //生成叶子节点
        if (numPrimitive == 1)
        {
            int firstOffset = mPrimitives.size();
            for (int i = start; i < end; ++i)
            {
                int index = primitiveInfos[i].index;
                mPrimitives.push_back(primitives[index]);
            }
            node->InitLeaf(firstOffset, numPrimitive, bound);
            return node;
        }
            //生成中间节点
        else
        {
            BBox cBound;
            for (int i = start; i < end; ++i)
            {
                cBound = Union(cBound, primitiveInfos[i].centroid);
            }
            //获取最大坐标轴
            int dim = cBound.maximumExtent();
            int mid = (end - start) / 2;
            if (cBound.mMax[dim] == cBound.mMin[dim])
            {
                int firstOffset = mPrimitives.size();
                for (int i = start; i < end; ++i)
                {
                    int index = primitiveInfos[i].index;
                    mPrimitives.push_back(primitives[index]);
                }
                node->InitLeaf(firstOffset, numPrimitive, bound);
                return node;
            }

            float cMid = (cBound.mMax[dim] + cBound.mMin[dim])*0.5f;
            BVHPrimitiveInfo* midPtr = std::partition(&primitiveInfos[start], &primitiveInfos[end - 1] + 1, [cMid, dim](const BVHPrimitiveInfo& info)
            {
                return info.centroid[dim] < cMid;
            });
            mid = midPtr - &primitiveInfos[0];

            node->InitInterior(recursiveBuild(primitiveInfos, start, mid, totalNodes, primitives), recursiveBuild(primitiveInfos, mid, end, totalNodes, primitives), dim);
        }

        return node;
    }

    int flattenBVHTree(std::shared_ptr<BVHBuildNode> node, int *offset)
    {
        LinearBVHNode* linearNode = &mNodes[*offset];
        linearNode->bound = node->bound;
        int myOffset = (*offset)++;
        if (node->numPrimitives > 0)
        {
            linearNode->primitiveOffset = node->firstPrimitiveOffset;
            linearNode->numPrimitive = node->numPrimitives;
        }
        else
        {
            linearNode->numPrimitive = 0;
            linearNode->axis = node->splitAxis;
            flattenBVHTree(node->children[0], offset);
            linearNode->secondChildOffset = flattenBVHTree(node->children[1], offset);
        }
        return myOffset;
    }
private:
    std::vector<std::shared_ptr<Triangle>> mPrimitives;
    LinearBVHNode* mNodes = nullptr;
    int mNodeCount;
};