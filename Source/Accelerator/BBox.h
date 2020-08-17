#ifndef STAR_BBOX_H
#define STAR_BBOX_H
#include <glm/glm.hpp>

namespace accel {
    class BBox
    {
    public:
        BBox();

        BBox(const glm::vec3& p);

        BBox(const glm::vec3& p1, const glm::vec3& p2);

        glm::vec3 center();

        glm::vec3 diagonal();

        float surfaceArea();

        float volume();

        int maximumExtent();

        void grow(const BBox& bbox);

        void grow(const glm::vec3& point);

        static BBox grow(const BBox& bbox1, const BBox& bbox2);

        static BBox grow(const BBox& bbox1, const glm::vec3& point);
    public:
        glm::vec3 mMin;
        glm::vec3 mMax;
    };
}
#endif
