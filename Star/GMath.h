#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <math.h>
namespace GMath
{
	static constexpr float Infinity = std::numeric_limits<float>::infinity();
	static constexpr float NegInfinity = -Infinity;
	static constexpr float Pi = 3.14159265358979323846;
	static constexpr float InvPi = 1.0f / Pi;
	static constexpr float TwoPi = 2.0f * Pi;
	static constexpr float InvTwoPi = 1.0f / TwoPi;
};

inline glm::vec3 TransformPoint(const glm::vec3& point, const glm::mat4& mat) 
{
	float x = point.x, y = point.y, z = point.z;
	float xp = mat[0][0] * x + mat[0][1] * y + mat[0][2] * z + mat[0][3];
	float yp = mat[1][0] * x + mat[1][1] * y + mat[1][2] * z + mat[1][3];
	float zp = mat[2][0] * x + mat[2][1] * y + mat[2][2] * z + mat[2][3];
	float wp = mat[3][0] * x + mat[3][1] * y + mat[3][2] * z + mat[3][3];
	assert(wp != 0);

	if (wp == 1.0f)
		return glm::vec3(xp, yp, zp);
	else
		return glm::vec3(xp, yp, zp) / wp;
}

inline glm::vec4 TransformPoint(const glm::vec4& point, const glm::mat4& mat) 
{
	float x = point.x, y = point.y, z = point.z, w = point.w;
	float xp = mat[0][0] * x + mat[0][1] * y + mat[0][2] * z + mat[0][3] * w;
	float yp = mat[1][0] * x + mat[1][1] * y + mat[1][2] * z + mat[1][3] * w;
	float zp = mat[2][0] * x + mat[2][1] * y + mat[2][2] * z + mat[2][3] * w;
	float wp = mat[3][0] * x + mat[3][1] * y + mat[3][2] * z + mat[3][3] * w;

	return glm::vec4(xp, yp, zp, wp);
}

inline glm::vec3 TransformVector(const glm::vec3& vec, const glm::mat4& mat)
{
	float x = vec.x, y = vec.y, z = vec.z;

	return glm::vec3(mat[0][0] * x + mat[0][1] * y + mat[0][2] * z,
		mat[1][0] * x + mat[1][1] * y + mat[1][2] * z,
		mat[2][0] * x + mat[2][1] * y + mat[2][2] * z);
}

inline glm::vec3 TransformNormal(const glm::vec3& norm, const glm::mat4& mat) 
{
	float x = norm.x, y = norm.y, z = norm.z;

	return glm::vec3(mat[0][0] * x + mat[1][0] * y + mat[2][0] * z,
		mat[0][1] * x + mat[1][1] * y + mat[2][1] * z,
		mat[0][2] * x + mat[1][2] * y + mat[2][2] * z);
}
