#include <iostream>
#include "Renderer.h"
#include "CommonMath.h"
#include "TriangleMesh.h"
#include "Importer.h"
#include "BVH.h"
int main() {
    SceneData* sceneData = loadScene("./Resources/Scenes/CornellBox.gltf");
    BVH* bvh = new BVH(sceneData->triangles);
    delete bvh;
    destroyScene(sceneData);
//    Renderer* renderer = new Renderer(512, 512);
//    renderer->initRenderer();
//    renderer->run();
//    delete renderer;
    return 0;
}