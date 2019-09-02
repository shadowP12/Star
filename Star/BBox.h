#pragma once
#include "GMath.h"
class BBox 
{
public:
	BBox() 
	{
		mMin = glm::vec3(GMath::Infinity, GMath::Infinity, GMath::Infinity);
		mMax = glm::vec3(GMath::NegInfinity, GMath::NegInfinity, GMath::NegInfinity);
	}

	BBox(const glm::vec3& p) 
	{
		mMin = p;
		mMax = p;
	}

	BBox(const glm::vec3& p1, const glm::vec3& p2) 
	{
		mMin = glm::vec3(glm::min(p1.x, p2.x), glm::min(p1.y, p2.y), glm::min(p1.z, p2.z));
		mMax = glm::vec3(glm::max(p1.x, p2.x), glm::max(p1.y, p2.y), glm::max(p1.z, p2.z));
	}
	//返回对角线向量
	glm::vec3 diagonal() const 
	{
		return (mMax - mMin);
	}

	//求面积
	float surfaceArea() const 
	{
		glm::vec3 d = diagonal();
		return (d.x * d.y + d.x * d.z + d.y * d.z) * 2;
	}

	//求体积
	float Volume() const 
	{
		glm::vec3 d = diagonal();
		return d.x * d.y * d.z;
	}

	//获取最大的边界
	int maximumExtent() const 
	{
		glm::vec3 diag = diagonal();
		if (diag.x > diag.y && diag.x > diag.z) 
		{
			return 0;
		}
		else if (diag.y > diag.z) 
		{
			return 1;
		}
		else 
		{
			return 2;
		}
	}
	const glm::vec3 &operator[](int i) const
	{
		return (&mMin)[i];
	}

	glm::vec3 &operator[](int i) 
	{
		return (&mMin)[i];
	}

	glm::vec3 mMin;
	glm::vec3 mMax;
};

inline BBox Union(const BBox& bbox1, const BBox& bbox2) 
{
	BBox ret;

	ret.mMin.x = glm::min(bbox1.mMin.x, bbox2.mMin.x);
	ret.mMin.y = glm::min(bbox1.mMin.y, bbox2.mMin.y);
	ret.mMin.z = glm::min(bbox1.mMin.z, bbox2.mMin.z);
	ret.mMax.x = glm::max(bbox1.mMax.x, bbox2.mMax.x);
	ret.mMax.y = glm::max(bbox1.mMax.y, bbox2.mMax.y);
	ret.mMax.z = glm::max(bbox1.mMax.z, bbox2.mMax.z);

	return ret;
}

inline BBox Union(const BBox& bbox, const glm::vec3& point) 
{
	BBox ret;

	ret.mMin.x = glm::min(bbox.mMin.x, point.x);
	ret.mMin.y = glm::min(bbox.mMin.y, point.y);
	ret.mMin.z = glm::min(bbox.mMin.z, point.z);
	ret.mMax.x = glm::max(bbox.mMax.x, point.x);
	ret.mMax.y = glm::max(bbox.mMax.y, point.y);
	ret.mMax.z = glm::max(bbox.mMax.z, point.z);

	return ret;
}
