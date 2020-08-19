#include "Renderer.h"
#include "Scene.h"
#include "Importer.h"
int main() {
//    star::Importer importer;
//    star::ImportedResult importedResult = importer.load("./Resources/Scenes/CornellBox.gltf");
    star::Scene* scene = new star::Scene;
    star::Renderer renderer(scene, 640, 640);
    renderer.prepare();
    renderer.runMainLoop();
    renderer.finish();
    delete scene;
    return 0;
}