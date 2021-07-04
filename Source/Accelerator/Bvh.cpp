#include "Bvh.h"
#include <algorithm>
#include <numeric>
namespace accel {
    Bvh::Bvh()
    {
    }

    Bvh::~Bvh()
    {
    }

    void Bvh::initNodeAllocator(uint32_t maxNum)
    {
        mNodeCount = 0;
        mNodes.resize(maxNum);
    }

    Bvh::Node* Bvh::allocateNode()
    {
        return &mNodes[mNodeCount++];
    }

    void Bvh::build(BBox *bounds, int numBound)
    {
        for (int i = 0; i < numBound; ++i)
        {
            mBound.grow(bounds[i]);
        }

        buildImpl(bounds, numBound);
    }

    void Bvh::buildImpl(BBox *bounds, int numBound)
    {
        initNodeAllocator(2 * numBound - 1);
        std::vector<PrimRef> primRefs(numBound);

        BBox centroidBound;
        for (int i = 0; i < numBound; ++i)
        {
            primRefs[i] = PrimRef{ bounds[i], bounds[i].center(), i };
            glm::vec3 c = bounds[i].center();
            centroidBound.grow(c);
        }

        SplitRequest init = { 0, numBound, mBound, centroidBound, 0 };
        mRoot = buildNode(init, primRefs);
    }

    Bvh::Node* Bvh::buildNode(SplitRequest &req, std::vector<PrimRef>& primRefs)
    {
        mHeight = glm::max(mHeight, req.level);

        Node* node = allocateNode();
        node->bound = req.bound;

        int numPrims = req.endIdx - req.startIdx;
        if (numPrims < 2)
        {
            node->type = NodeType::Leaf;
            node->startIdx = mPackedIndices.size();
            node->numPrims = numPrims;
            for (int i = req.startIdx; i < req.endIdx; ++i)
            {
                mPackedIndices.push_back(primRefs[i].idx);
            }
            return node;
        }
        else
        {
            int dim = req.centroidBound.maximumExtent();
            int mid = (req.endIdx - req.startIdx) / 2;
            if (req.centroidBound.mMax[dim] == req.centroidBound.mMin[dim])
            {
                node->type = NodeType::Leaf;
                node->startIdx = mPackedIndices.size();
                node->numPrims = numPrims;
                for (int i = req.startIdx; i < req.endIdx; ++i)
                {
                    mPackedIndices.push_back(primRefs[i].idx);
                }
                return node;
            }
            float midValue = (req.centroidBound.mMax[dim] + req.centroidBound.mMin[dim]) * 0.5f;
            PrimRef* midPtr = std::partition(&primRefs[req.startIdx], &primRefs[req.endIdx - 1] + 1, [midValue, dim](const PrimRef& info)
            {
                return info.center[dim] < midValue;
            });
            int midIdx = midPtr - &primRefs[0];

            BBox leftBound, rightBound, leftCentroidBound, rightCentroidBound;
            for (int i = req.startIdx; i < midIdx; ++i)
            {
                leftBound.grow(primRefs[i].bound);
                leftCentroidBound.grow(primRefs[i].center);
            }

            for (int i = midIdx; i < req.endIdx; ++i)
            {
                rightBound.grow(primRefs[i].bound);
                rightCentroidBound.grow(primRefs[i].center);
            }

            // Left request
            SplitRequest leftrequest = { req.startIdx, midIdx, leftBound, leftCentroidBound, req.level + 1 };
            // Right request
            SplitRequest rightrequest = { midIdx, req.endIdx, rightBound, rightCentroidBound, req.level + 1 };

            node->type = NodeType::Internal;
            node->lc = buildNode(leftrequest, primRefs);
            node->rc = buildNode(rightrequest, primRefs);
        }
        return node;
    }
}