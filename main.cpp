#include <iostream>
#include "Renderer.h"
#include "CommonMath.h"
#include "TriangleMesh.h"
#include "Importer.h"
#include "BVH.h"
int main() {
    SceneData* sceneData = loadScene("./Resources/Scenes/CornellBox.gltf");
    BVH* bvh = new BVH(sceneData->triangles);
    Renderer* renderer = new Renderer(512, 512);
    renderer->initRenderer(bvh, sceneData->materials);
    renderer->run();
    delete renderer;
    delete bvh;
    destroyScene(sceneData);
    return 0;
}