#pragma once
#include "Content/Shared/Draw/draw_buffer.h"

class AxesDrawBuffer : public DrawBuffer {
public:
	AxesDrawBuffer();

	void render(DrawProgram* program, QOpenGLFunctions* fn) override;

private:
	GLsizei _gridVertexCount{ 0 };
};
