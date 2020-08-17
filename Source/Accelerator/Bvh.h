#ifndef STAR_BVH_H
#define STAR_BVH_H
#include <vector>
#include "BBox.h"

namespace accel {
    class Bvh
    {
        enum class NodeType
        {
            Internal,
            Leaf
        };

        struct Node
        {
            BBox bound;
            NodeType type;
            union
            {
                struct
                {
                    Node* lc;
                    Node* rc;
                };

                struct
                {
                    int startIdx;
                    int numPrims;
                };
            };
        };

        struct PrimRef
        {
            BBox bound;
            glm::vec3 center;
            int idx;
        };

        struct SplitRequest
        {
            int startIdx;
            int endIdx;
            BBox bound;
            BBox centroidBound;
            int level;
        };
    public:
        Bvh();
        ~Bvh();
        void build(BBox* bounds, int numBound);
    protected:
        virtual void initNodeAllocator(uint32_t maxNum);
        virtual Node* allocateNode();
        virtual void buildImpl(BBox* bounds, int numBound);
        Node* buildNode(SplitRequest& req, std::vector<PrimRef>& primRefs);
    protected:
        Node* mRoot;
        uint32_t mNodeCount;
        int mHeight;
        BBox mBound;
        std::vector<uint32_t> mIndices;
        std::vector<uint32_t> mPackedIndices;
        std::vector<Node> mNodes;
    };
}

#endif
