#include "Content/MapEditor/DrawBuffers/grid_buffer.h"
#include "Content/MapEditor/Draw/draw_program.h"
#include <QOpenGLFunctions>

GridDrawBuffer::GridDrawBuffer()
{
	std::vector<float> gridVerts;
	for (int i = 0; i <= 10; i++)
	{
		float f = static_cast<float>(i);
		gridVerts.insert(gridVerts.end(), {
			0.0f, 0.0f, f,
			10.0f, 0.0f, f,
			f, 0.0f, 0.0f,
			f, 0.0f, 10.0f
			});
	}

	_gridVertexCount = gridVerts.size();

	create(gridVerts, { {0, 3} });
}

void GridDrawBuffer::render(DrawProgram* program, QOpenGLFunctions* fn)
{
	program->setUniformValue("uColor", QVector3D(0.4f, 0.4f, 0.4f));
	fn->glDrawArrays(GL_LINES, 0, _gridVertexCount);
}
