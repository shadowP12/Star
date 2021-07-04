#ifndef STAR_IMPORTER_H
#define STAR_IMPORTER_H
#include <vector>
#include <string>
#include <map>
namespace star {
    class Mesh;
    struct MeshInstance;

    struct ImportedResult
    {
        std::vector<Mesh*> meshs;
        std::vector<MeshInstance> meshInstances;
    };

    class Importer
    {
    public:
        Importer();
        ~Importer();
        ImportedResult load(std::string path);
    };
}

#endif