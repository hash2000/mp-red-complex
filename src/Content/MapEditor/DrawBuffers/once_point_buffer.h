#pragma once
#include "Content/MapEditor/Draw/draw_buffer.h"

class OncePointDrawBuffer : public DrawBuffer {
public:
	OncePointDrawBuffer();

	void render(DrawProgram* program, QOpenGLFunctions* fn) override;
};
