#pragma once

#include "CommonMath.h"
#include <vector>
#include <string>

struct ShaderDesc
{
    int type;// 0:vert 1:frag 2:compute
    std::string source;
    std::string includeDir;
};

std::vector<uint32_t> compileShader(const ShaderDesc& desc);

class Camera
{
public:
    Camera(glm::vec3 position, float yaw = -90.0f, float pitch = 0.0f);
    ~Camera();
    void move(float wheel);
    void rotate(glm::vec2 offset);
    glm::vec3 getPosition() { return mPosition; }
    glm::vec3 getFront(){ return mFront; }
    glm::vec3 getUp(){ return mUp; }
    glm::vec3 getRight(){ return mRight; }
private:
    void updateVectors();
private:
    glm::vec3 mPosition;
    glm::vec3 mFront;
    glm::vec3 mUp;
    glm::vec3 mRight;
    float mYaw;
    float mPitch;
};