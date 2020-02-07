#pragma once
#include "Model.h"
#include "ShaderProgram.h"
#include "Primitive.h"
#include "Math/GMath.h"

struct SceneNode
{
    std::string name;
    int id;
    int parent;
    std::vector<int> childrens;
    glm::vec3 translation;
    glm::vec3 scale;
    glm::quat rotation;
};

class Scene
{
public:
	Scene();
	~Scene();
	void load(std::string file);
	void genPrimitives();
	std::vector<std::shared_ptr<Primitive>>& getPrimitives();
	void draw(std::shared_ptr<ShaderProgram> sp);
    glm::mat4 getLocalMatrix(int id);
    glm::mat4 getWorldMatrix(int id);
private:
	std::vector<std::shared_ptr<Model>> mModels;
	std::vector<std::shared_ptr<Primitive>> mPrimitives;
	std::vector<SceneNode> mNodes;
};