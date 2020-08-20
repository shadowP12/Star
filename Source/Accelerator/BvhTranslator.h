#ifndef STAR_BVHTRANSLATOR_H
#define STAR_BVHTRANSLATOR_H

#include "BBox.h"
#include "Bvh.h"

namespace accel {
    struct BvhInstance
    {
        int bvhIdx;
        glm::mat4 transform;
    };

    class BvhTranslator
    {
    public:
        struct Node
        {
            alignas(16) glm::vec3 bboxMin;
            alignas(16) glm::vec3 bboxMax;
            alignas(4) int leaf;
            alignas(4) int leftIndex;
            alignas(4) int rightIndex;
        };
    public:
        BvhTranslator();
        ~BvhTranslator();
        void process(Bvh* topBvh, std::vector<Bvh*> bvhs, std::vector<BvhInstance> bvhInstances);
        void processBLAS();
        int processBLASNodes(Bvh::Node* node);
        void processTLAS();
        int processTLASNodes(Bvh::Node* node);

    public:
        std::vector<Node> mNodes;
        std::vector<int> mBvhRootStartIndices;
        int mCurNodeIndex = 0;
        int mCurPrimIndex = 0;
        int mTopIndex = 0;
        Bvh* mTopBvh;
        std::vector<Bvh*> mBvhs;
        std::vector<BvhInstance> mBvhInstances;
    };
}

#endif