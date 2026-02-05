#pragma once
#include "Content/MapEditor/Draw/draw_buffer.h"

class GridDrawBuffer : public DrawBuffer {
public:
	GridDrawBuffer();

	void render(DrawProgram* program, QOpenGLFunctions* fn) override;

private:
	GLsizei _gridVertexCount{0};
};
