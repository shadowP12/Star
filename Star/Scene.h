#pragma once
#include "Model.h"
#include "ShaderProgram.h"
#include "Primitive.h"
#include "Math/GMath.h"

struct Material
{
    glm::vec3 baseColor;
    glm::vec3 emissive;
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
private:
	std::vector<std::shared_ptr<Model>> mModels;
	std::vector<std::shared_ptr<Material>> mMaterials;
	std::vector<std::shared_ptr<Primitive>> mPrimitives;
	std::vector<std::shared_ptr<Node>> mNodes;
};