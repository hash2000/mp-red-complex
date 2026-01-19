#include "Content/Shared/DrawBuffers/once_point_buffer.h"
#include "Content/Shared/Draw/draw_program.h"
#include <QOpenGLFunctions>

OncePointDrawBuffer::OncePointDrawBuffer()
{
	std::vector<float> data = { 0.0f, 0.0f, 0.0f };
	create(data, { {0, 3} });
}

void OncePointDrawBuffer::render(DrawProgram* program, QOpenGLFunctions* fn)
{
	program->setUniformValue("uColor", QVector3D(1.0f, 1.0f, 0.0f));
	fn->glDrawArrays(GL_POINTS, 0, 1);
}
