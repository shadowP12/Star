#pragma once

#include <string>
#include <memory>
#include <vector>

class Triangle;
class TriangleMesh;
class Material;

struct SceneData
{
    std::vector<std::shared_ptr<TriangleMesh>> triangleMeshs;
    std::vector<std::shared_ptr<Triangle>> triangles;
    std::vector<std::shared_ptr<Material>> materials;
};

SceneData* loadScene(std::string path);

void destroyScene(SceneData* data);