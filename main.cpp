#include "Renderer.h"
#include "Scene.h"
#include "Importer.h"
int main() {
    star::Importer importer;
    star::ImportedResult importedResult = importer.load("./Resources/Scenes/CornellBox.gltf");
    star::Scene* scene = new star::Scene;
    {
        star::Light light;
        light.type = 0;
        light.position = glm::vec3(-0.25f, 1.9f, 0.0f);
        light.u = glm::vec3(0.25f, 1.9f, 0.0f) - light.position;
        light.v = glm::vec3(-0.25f, 1.9f, 0.5f) - light.position;
        light.area = glm::length(glm::cross(light.u, light.v));
        light.emission = glm::vec3(10, 10, 10);
        scene->addLight(light);

//        light.type = 1;
//        light.position = glm::vec3(0.0f, 1.9f, 0.0f);
//        light.emission = glm::vec3(10, 10, 10);
//        light.radius = 0.2f;
//        light.area = 4 * 3.1415926 * (light.radius * light.radius);
//        scene->addLight(light);
    }
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