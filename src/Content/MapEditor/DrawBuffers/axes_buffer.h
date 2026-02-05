#pragma once
#include "Content/MapEditor/Draw/draw_buffer.h"

class AxesDrawBuffer : public DrawBuffer {
public:
	AxesDrawBuffer();

	void render(DrawProgram* program, QOpenGLFunctions* fn) override;

private:
	GLsizei _gridVertexCount{ 0 };
};
