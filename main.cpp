#include "Renderer.h"
#include "Scene.h"
#include "Importer.h"
int main() {
    star::Importer importer;
    star::ImportedResult importedResult = importer.load("./Resources/Scenes/CornellBox.gltf");
    star::Scene* scene = new star::Scene;
    for (int i = 0; i < importedResult.meshs.size(); ++i)
    {
        scene->addMesh(importedResult.meshs[i]);
    }
    for (int i = 0; i < importedResult.meshInstances.size(); ++i)
    {
        scene->addMeshInstance(importedResult.meshInstances[i]);
    }
    scene->createAccelerationStructures();
    star::Renderer renderer(scene, 640, 640);
    renderer.prepare();
    renderer.runMainLoop();
    renderer.finish();
    delete scene;
    return 0;
}