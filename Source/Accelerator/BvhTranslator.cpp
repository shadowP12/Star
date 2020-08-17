#include "BvhTranslator.h"
namespace accel {
    BvhTranslator::BvhTranslator()
    {
    }

    BvhTranslator::~BvhTranslator()
    {
    }

    void BvhTranslator::process(Bvh* topBvh, std::vector<Bvh*> bvhs, std::vector<BvhInstance> bvhInstances)
    {
        mTopBvh = topBvh;
        mBvhs = bvhs;
        mBvhInstances = bvhInstances;
        processBLAS();
        processTLAS();
    }

    void BvhTranslator::processBLAS()
    {
        int nodeCount = 0;

        for (int i = 0; i < mBvhs.size(); i++)
            nodeCount += mBvhs[i]->mNodeCount;
        mTopIndex = nodeCount;
        nodeCount += 2 * mBvhInstances.size();
        mNodes.resize(nodeCount);

        int bvhRootIndex = 0;
        mCurPrimIndex = 0;

        for (int i = 0; i < mBvhs.size(); i++)
        {
            Bvh* bvh = mBvhs[i];
            mCurNodeIndex = bvhRootIndex;

            mBvhRootStartIndices.push_back(bvhRootIndex);
            bvhRootIndex += bvh->mNodeCount;

            processBLASNodes(bvh->mRoot);
            mCurPrimIndex += bvh->getNumIndices();
        }
    }

    int BvhTranslator::processBLASNodes(Bvh::Node *node)
    {
        BBox bound = node->bound;
        mNodes[mCurNodeIndex].bboxMin = bound.mMin;
        mNodes[mCurNodeIndex].bboxMax = bound.mMax;
        mNodes[mCurNodeIndex].leaf = 0;

        int index = mCurNodeIndex;

        if(node->type == Bvh::NodeType::Leaf)
        {
            mNodes[mCurNodeIndex].leftIndex = mCurPrimIndex + node->startIdx;
            mNodes[mCurNodeIndex].rightIndex = node->numPrims;
            mNodes[mCurNodeIndex].leaf = 1;
        }
        else
        {
            mCurNodeIndex++;
            mNodes[index].leftIndex = processBLASNodes(node->lc);
            mCurNodeIndex++;
            mNodes[index].rightIndex = processBLASNodes(node->rc);
        }
        return index;
    }

    void BvhTranslator::processTLAS()
    {
        mCurNodeIndex = mTopIndex;
        processTLASNodes(mTopBvh->mRoot);
    }

    int BvhTranslator::processTLASNodes(Bvh::Node *node)
    {
        BBox bound = node->bound;
        mNodes[mCurNodeIndex].bboxMin = bound.mMin;
        mNodes[mCurNodeIndex].bboxMax = bound.mMax;
        mNodes[mCurNodeIndex].leaf = 0;

        int index = mCurNodeIndex;

        if(node->type == Bvh::NodeType::Leaf)
        {
            int instanceIndex = mTopBvh->mPackedIndices[node->startIdx];
            mNodes[mCurNodeIndex].leftIndex = mBvhRootStartIndices[mBvhInstances[instanceIndex].bvhIdx];
            mNodes[mCurNodeIndex].rightIndex = instanceIndex;
            mNodes[mCurNodeIndex].leaf = 2;
        }
        else
        {
            mCurNodeIndex++;
            mNodes[index].leftIndex = processTLASNodes(node->lc);
            mCurNodeIndex++;
            mNodes[index].rightIndex = processTLASNodes(node->rc);
        }
        return index;
    }
}
