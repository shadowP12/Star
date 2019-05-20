#pragma once
#include "Model.h"
#include "ShaderProgram.h"
#include "Primitive.h"
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
	std::vector<std::shared_ptr<Primitive>> mPrimitives;
};