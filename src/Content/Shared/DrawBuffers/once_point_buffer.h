#pragma once
#include "Content/Shared/Draw/draw_buffer.h"

class OncePointDrawBuffer : public DrawBuffer {
public:
	OncePointDrawBuffer();

	void render(DrawProgram* program, QOpenGLFunctions* fn) override;
};
