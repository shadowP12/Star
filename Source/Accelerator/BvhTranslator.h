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
        struct Node
        {
            int leaf;
            int leftIndex;
            int rightIndex;
            glm::vec3 bboxMin;
            glm::vec3 bboxMax;
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