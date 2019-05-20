#pragma once
#include "Module.h"
#define GLEW_STATIC
#include <glew/include/GL/glew.h>
#include "ShaderProgram.h"
#define MAX_APG_GL_DB_LINES 819200
class DebugDraw : public Module<DebugDraw>
{
public:
	DebugDraw()
	{
		glGenVertexArrays(1, &mVAO);
		glBindVertexArray(mVAO);
		glGenBuffers(1, &mVBO);
		glBindBuffer(GL_ARRAY_BUFFER, mVBO);
		glBufferData(GL_ARRAY_BUFFER, 4 * MAX_APG_GL_DB_LINES * 14, NULL, GL_DYNAMIC_DRAW);

		GLsizei stride = 4 * 7;
		GLintptr offs = 4 * 3;
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, NULL);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*)offs);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);
	}
	~DebugDraw()
	{
		glDeleteBuffers(1, &mVAO);
		glDeleteVertexArrays(1, &mVBO);
	}

	void draw()
	{
		glBindVertexArray(mVAO);
		glDrawArrays(GL_LINES, 0, 2 * mLinesCount);
		glBindVertexArray(0);
	}

	int addLine(float* start_xyz, float* end_xyz, float* colour_rgba) 
	{
		if (mLinesCount >= MAX_APG_GL_DB_LINES) 
		{
			std::cout << "ERROR: too many gl db lines" << std::endl;
			return -1;
		}

		float sd[14];
		sd[0] = start_xyz[0];
		sd[1] = start_xyz[1];
		sd[2] = start_xyz[2];
		sd[3] = colour_rgba[0];
		sd[4] = colour_rgba[1];
		sd[5] = colour_rgba[2];
		sd[6] = colour_rgba[3];
		sd[7] = end_xyz[0];
		sd[8] = end_xyz[1];
		sd[9] = end_xyz[2];
		sd[10] = colour_rgba[0];
		sd[11] = colour_rgba[1];
		sd[12] = colour_rgba[2];
		sd[13] = colour_rgba[3];

		glBindBuffer(GL_ARRAY_BUFFER, mVBO);
		GLintptr os = sizeof(sd) * mLinesCount;
		GLsizei sz = sizeof(sd);
		glBufferSubData(GL_ARRAY_BUFFER, os, sz, sd);

		return mLinesCount++;
	}
	int addBox(float* min_xyz, float* max_xyz, float* colour_rgba) 
	{
		int rid = -1;
		float start[3], end[3];

		// bottom ring
		// rear
		start[0] = min_xyz[0]; // L
		end[0] = max_xyz[0]; // R
		start[1] = end[1] = min_xyz[1]; // B
		start[2] = end[2] = min_xyz[2]; // P
		rid = addLine(start, end, colour_rgba);
		// right
		start[0] = max_xyz[0]; // R
		end[2] = max_xyz[2]; // A
		addLine(start, end, colour_rgba);
		// front
		end[0] = min_xyz[0]; // L
		start[2] = max_xyz[2]; // A
		addLine(start, end, colour_rgba);
		// left
		start[0] = min_xyz[0]; // L
		end[2] = min_xyz[2]; // P
		addLine(start, end, colour_rgba);

		// top ring
		start[0] = min_xyz[0]; // L
		end[0] = max_xyz[0]; // R
		start[1] = end[1] = max_xyz[1]; // T
		start[2] = end[2] = min_xyz[2]; // P
		addLine(start, end, colour_rgba);
		// right
		start[0] = max_xyz[0]; // R
		end[2] = max_xyz[2]; // A
		addLine(start, end, colour_rgba);
		// front
		end[0] = min_xyz[0]; // L
		start[2] = max_xyz[2]; // A
		addLine(start, end, colour_rgba);
		// left
		start[0] = min_xyz[0]; // L
		end[2] = min_xyz[2]; // P
		addLine(start, end, colour_rgba);

		// 4 side edges
		start[0] = end[0] = min_xyz[0]; // L
		start[1] = min_xyz[1];
		end[1] = max_xyz[1];
		start[2] = end[2] = min_xyz[2]; // P
		addLine(start, end, colour_rgba);
		start[0] = end[0] = max_xyz[0]; // R
		addLine(start, end, colour_rgba);
		start[2] = end[2] = max_xyz[2]; // A
		addLine(start, end, colour_rgba);
		start[0] = end[0] = min_xyz[0]; // L
		addLine(start, end, colour_rgba);

		return rid;
	}
private:
	GLuint mVAO, mVBO;
	uint32_t mLinesCount = 0;//当前需要绘制线的数量
};